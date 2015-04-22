#include <yak/kernel.h>
#include <yak/initcall.h>
#include <yak/lib/string.h>
#include <yak/lib/utils.h>
#include <yak/lib/sort.h>
#include <yak/arch/acpi.h>
#include <yak/arch/ports.h>
#include <yak/arch/pit.h>
#include <yak/arch/pic.h>
#include <yak/arch/lapic.h>
#include <yak/arch/ioapic.h>
#include <yak/mem/vmm.h>
#include <yak/mem/pmm.h>
#include <yak/mem/mem.h>
#include <yak/cpu/idt.h>
#include <yak/cpu/gdt.h>
#include <yak/cpu/percpu.h>
#include <yak/cpu/mp.h>

#define LOG "\33\x0a\xf0smp   ::\33r"

typedef struct
{
    uint32_t lapic_address;
    uint32_t flags;
} __packed acpi_madt_t;

#define PCAT_COMPAT 1

typedef struct
{
    uint8_t entry_type;
    uint8_t record_length;
} __packed madt_record_t;

typedef struct
{
    uint8_t acpi_proc_id;
    uint8_t lapic_id;
    uint32_t flags;
} __packed madt_lapic_t;

#define ENABLED 1

typedef struct
{
    uint8_t id;
    uint8_t reserved;
    uint32_t address;
    uint32_t interrupt_base;
} __packed madt_ioapic_t;

typedef struct
{
    uint8_t bus_source;
    uint8_t irq_source;
    uint32_t interrupt;
    uint16_t flags;
} __packed madt_int_override_t;

typedef struct
{
    uint8_t acpi_proc_id;
    uint16_t flags;
    uint8_t lapic_lintn;
} __packed madt_lapic_nmi_t;

typedef struct
{
    uint8_t reserved[2];
    uint64_t address;
} __packed madt_lapic_address_t;

enum { 
    MADT_LAPIC                  = 0,
    MADT_IOAPIC                 = 1,
    MADT_INT_SRC_OVERRIDE       = 2,
    MADT_NMI                    = 3,
    MADT_LAPIC_NMI              = 4,
    MADT_LAPIC_ADDRESS_OVERRIDE = 5,
    MADT_IOSAPIC                = 6,
    MADT_LSAPIC                 = 7,
    MADT_PLATFORM_INT_SOURCES   = 8,
    MADT_PROC_LOCAL_X2APIC      = 9,
    MADT_LOCAL_X2APIC_NMI       = 0xa,
    MADT_GIC                    = 0xb,
    MADT_GICD                   = 0xc
};

const char *madt_entry_label[] = {
    [0x0] = "Processor Local APIC",
    [0x1] = "I/O APIC",
    [0x2] = "Interrupt Source Override",
    [0x3] = "Non-Maskable Interrupt Source (NMI)",
    [0x4] = "Local APIC NMI",
    [0x5] = "Local APIC Address Override",
    [0x6] = "I/O SAPIC",
    [0x7] = "Local SAPIC",
    [0x8] = "Platform Interrupt Sources",
    [0x9] = "Processor Local x2APIC",
    [0xa] = "Local x2APIC NMI",
    [0xb] = "GIC",
    [0xc] = "GICD"
};

struct mp_params
{
    uintptr_t cr3;
    gdt_ptr_t gdt_ptr;
    idt_ptr_t idt_ptr;
    uintptr_t stack_ptr;
    uintptr_t percpu_ptr;
    unsigned int id;
} __packed;

struct madt_info
{
    uintptr_t madt_address;
    unsigned int total_cores;
    unsigned int enabled_cores;
    uint64_t lapic_address;
    uint32_t madt_flags;
};

static struct madt_info madt_info;
static unsigned int cores_alive = 1;
static unsigned int next_proc_id = 1;

void ap_main(unsigned int id, uintptr_t percpu_base)
{
    percpu_init(id, percpu_base);
    lapic_enable(0);
    *(unsigned char *)(VMM_P2V(AP_STATUS_FLAG)) = AP_READY;
    local_irq_enable();

    //uintptr_t frame = alloc_frame();
    //printk("frame %08x%08x\n", frame >> 32, frame);
    //print_mem_stat_local();

    for (;;)
        cpu_relax();
}

#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71
#define LAPIC_VER_NEW 0x10

void start_ap(unsigned int proc_id, unsigned int lapic_id, uintptr_t addr)
{
    //printk(LOG " starting core #%u (lapic #%u)\n", proc_id, lapic_id);

    // The BSP must initialize CMOS shutdown code to 0x0a...
    outb(CMOS_ADDRESS, 0xf); // offset 0xf is shutdown code
    outb(CMOS_DATA, 0xa);
    
    // and the warn reset vector (dword based at 40:67) to point
    // to the AP startup code
    *((volatile unsigned *)VMM_P2V(0x467)) = (addr & 0xff000) << 12;

    // reset the AP status
    unsigned char *ap_status = (unsigned char *)VMM_P2V(AP_STATUS_FLAG);
    *ap_status = AP_SLEEP;

    lapic_clear_error();

    lapic_send_init_ipi(lapic_id);
    pit_udelay(10000); // wait 10ms

    if (lapic_version() >= LAPIC_VER_NEW) {
        lapic_send_startup_ipi(lapic_id, addr);
        for (int i = 0; i < 10 && *ap_status != AP_STARTED; ++i);
            pit_udelay(100); // wait a total of 1ms

        if (*ap_status != AP_STARTED) {
            lapic_send_startup_ipi(lapic_id, addr);
            for (int i = 0; i < 10000 && *ap_status != AP_STARTED; ++i) {
                pit_udelay(100); // wait a total of 1s
            }
        }
    }
    if (*ap_status != AP_STARTED) {
        *ap_status = AP_READY; // don't block bsp
        printk(LOG "\33\x0f\x40 Unable to start core %u with lapic id %u\n", proc_id, lapic_id);
    } else {
        *ap_status = AP_CONTINUE;
        ++cores_alive;
    }

    lapic_clear_error();

    // clean up BIOS reset vector
    outb(CMOS_ADDRESS, 0xf);
    outb(CMOS_DATA, 0);
}

INIT_CODE void madt_get_info(uintptr_t madt_address, struct madt_info *info)
{
    info->total_cores = 0;
    info->enabled_cores = 0;
    info->madt_address = madt_address;

    if (madt_address == 0)
        return;

    sdt_header_t *madt_header = (sdt_header_t *)map_temp(madt_address);
    acpi_madt_t *madt_data = (acpi_madt_t *)((uint8_t *)madt_header + sizeof(sdt_header_t));

    info->lapic_address = madt_data->lapic_address;
    info->madt_flags = madt_data->flags;
    
    uint8_t *record = (uint8_t *)madt_data + sizeof(acpi_madt_t);

    while (record < (uint8_t *)madt_header + madt_header->length) {
        switch (*record) {
        case MADT_LAPIC:
            ++info->total_cores;
            madt_lapic_t *lapic_record = (madt_lapic_t *)(record + sizeof(madt_record_t));
            if (lapic_record->flags & ENABLED)
                ++info->enabled_cores;
            break;

        case MADT_LAPIC_ADDRESS_OVERRIDE: ;
            madt_lapic_address_t *entry = (madt_lapic_address_t *)(record + sizeof(madt_record_t));
            info->lapic_address = entry->address;
            break;
        }
        record += *(record + 1);
    }
    printk(LOG " detected %u core(s) - %u enabled\n", info->total_cores, info->enabled_cores);
}

extern const char trampoline[];
extern const char trampoline_end[];
extern const char kernel_percpu_start[];
extern const char kernel_percpu_end[];

static uintptr_t percpu_areas;
static uintptr_t stacks;

// This has to run before mem_init() and before mp_init1().
// This function counts the cpus and allocate space for the
// percpu areas and stacks just after BSP percpu area.
// The BSP percpu area is also initialized.
INIT_CODE void mp_init0(uintptr_t madt_address)
{
    madt_get_info(madt_address, &madt_info);

    percpu_init(0, (uintptr_t)kernel_percpu_start);

    // create the percpu areas and stacks right after the bsp percpu area
    mem_percpu_init(madt_info.enabled_cores - 1, &percpu_areas, &stacks);
}

static INIT_CODE void trampoline_init(struct mp_params *params)
{
    memcpy((void *)VMM_P2V(TRAMPOLINE_START), (void *)trampoline, trampoline_end - trampoline);

    asm volatile("movq %%cr3, %0" : "=r"(params->cr3));
    asm volatile("sgdt %0" : "=m"(params->gdt_ptr));
    asm volatile("sidt %0" : "=m"(params->idt_ptr));
}

INIT_CODE void mp_init1(void)
{
    if (madt_info.madt_flags & PCAT_COMPAT)
        pic_disable();

    lapic_init(madt_info.lapic_address);
    const uint8_t bsp_id = lapic_id();

    const size_t percpu_size = 
        align_up((uintptr_t)kernel_percpu_end - (uintptr_t)kernel_percpu_start, PAGE_SIZE);

    unsigned char *ap_status = (unsigned char *)VMM_P2V(AP_STATUS_FLAG);

    struct mp_params params;
    trampoline_init(&params);

    sdt_header_t *madt_header = (sdt_header_t *)map_temp(madt_info.madt_address);
    acpi_madt_t *madt_data = (acpi_madt_t *)((uint8_t *)madt_header + sizeof(sdt_header_t));
    uint8_t *record = (uint8_t *)madt_data + sizeof(acpi_madt_t);

    while (record < (uint8_t *)madt_header + madt_header->length) {
        switch (*record) {
            case MADT_LAPIC: ;
                madt_lapic_t *lapic_record = (madt_lapic_t *)(record + sizeof(madt_record_t));
                //printk("lapic id %u acpi id %u\n", lapic_record->lapic_id, lapic_record->acpi_proc_id);
                if ((lapic_record->flags & ENABLED) && lapic_record->lapic_id != bsp_id) {

                    params.id = next_proc_id++;
                    params.stack_ptr = stacks + params.id * STACK_SIZE - 8;
                    params.percpu_ptr = percpu_areas + (params.id - 1) * percpu_size;
                    memcpy((void *)TRAMPOLINE_PARAMS, (void *)&params, sizeof(params));

                    start_ap(params.id, lapic_record->lapic_id, TRAMPOLINE_START);
                    while (*ap_status != AP_READY)
                        cpu_relax();
                }
                break;

            case MADT_IOAPIC: ;
                madt_ioapic_t *ioapic_record = (madt_ioapic_t *)(record + sizeof(madt_record_t));
                ioapic_add(ioapic_record->id, ioapic_record->address, ioapic_record->interrupt_base);
                break;

            case MADT_INT_SRC_OVERRIDE: ;
                madt_int_override_t *override = (madt_int_override_t *)(record + sizeof(madt_record_t));
                printk(LOG " ignoring INT OVERRIDE entry: bus = %u, source = %u, int = %u, flags = %#04x\n",
                        override->bus_source, override->irq_source, override->interrupt, override->flags);
                break;

            case MADT_LAPIC_NMI: ;
                madt_lapic_nmi_t *lapic_nmi = (madt_lapic_nmi_t *)(record + sizeof(madt_record_t));
                printk(LOG " ignoring LAPIC NMI: acpi proc id = %u, flags = %04x, lapic LINTn = %u\n",
                        lapic_nmi->acpi_proc_id, lapic_nmi->flags, lapic_nmi->lapic_lintn);
                break;

            default: ;
                printk(LOG " \33\x0f\x40Skipping MADT entry: %s [%u]\n", 
                        *record >= 0xd ? "Reserved" : madt_entry_label[*record], *record);
                break;
        }
        record += *(record + 1); // add length (field 1)
    }
    printk(LOG " %u cores have been started\n", cores_alive);

    local_irq_enable();
}
