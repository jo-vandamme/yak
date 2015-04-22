#include <yak/kernel.h>
#include <yak/mem/pmm.h>
#include <yak/mem/vmm.h>
#include <yak/arch/ioapic.h>

#define LOG "\33\x0a\xf0ioapic::\33r"

#define MAX_IOAPICS 8 // arbitrary

#define IOREGSEL    0x00
#define IOREGWIN    0x10

#define IOAPICID    0x00
#define IOAPICVER   0x01
#define IOAPICARB   0x02
#define IOREDTBL    0x10

char ioapic_mapping[PAGE_SIZE * MAX_IOAPICS] __attribute__((aligned(PAGE_SIZE)));

typedef struct ioapic
{
    uint8_t id;
    uint8_t version;
    uintptr_t phys_base;
    uintptr_t virt_base;
    uint32_t int_base;
    uint8_t int_max;
} ioapic_t;

// empty bit-fields are marked as reserved in the specification
union ioapicid {
    struct {
        uint32_t    : 24;
        uint32_t id : 4;
        uint32_t    : 4;
    };
    uint32_t raw;
};

union ioapicver {
    struct {
        uint32_t version       : 8;
        uint32_t               : 8;
        uint32_t max_red_entry : 8;
        uint32_t               : 8;
    };
    uint32_t raw;
};

union ioapicarb {
    struct {
        uint32_t    : 24;
        uint32_t id : 4;
        uint32_t    : 4;
    };
    uint32_t raw;
};

// destination modes
#define PHYSICAL_MODE   0
#define LOGICAL_MODE    1

// delivery mode
#define FIXED           0
#define LOWEST_PRIORITY 1
#define SMI             2
#define NMI             4
#define INIT            5
#define EXTINT          7

union ioredtbl {
    struct {
        uint64_t vector             : 8;
        uint64_t delivery_mode      : 3;
        uint64_t destination_mode   : 1;
        uint64_t delivery_status    : 1;
        uint64_t input_pin_polarity : 1;
        uint64_t remote_irr         : 1;
        uint64_t trigger_mode       : 1;
        uint64_t mask               : 1;
        uint64_t                    : 39;
        uint64_t destination        : 8;
    };
    struct {
        uint32_t low;
        uint32_t high;
    } __attribute__((packed));
};

static unsigned int num_ioapics = 0;
static ioapic_t ioapics[MAX_IOAPICS]; 

void ioapic_write(const uintptr_t ioapic_base, const uint8_t offset, const uint32_t val)
{
    *(uint8_t *)(ioapic_base + IOREGSEL) = offset;
    mem_barrier();
    *(uint32_t *)(ioapic_base + IOREGWIN) = val;
    mem_barrier();
}

uint32_t ioapic_read(const uintptr_t ioapic_base, const uint8_t offset)
{
    *(uint8_t *)(ioapic_base + IOREGSEL) = offset;
    mem_barrier();
    uint32_t val = *(uint32_t *)(ioapic_base + IOREGWIN);
    mem_barrier();
    return val;
}

static void ioapic_mask_irq(const uint8_t irq, const uint8_t ioapic_id)
{
    const uint32_t low_index  = IOREDTBL + irq * 2;
    const uint32_t high_index = IOREDTBL + irq * 2 + 1;

    union ioredtbl reg;
    reg.low = ioapic_read(ioapics[ioapic_id].virt_base, low_index);
    reg.high = ioapic_read(ioapics[ioapic_id].virt_base, high_index);
    reg.mask = 1;
    ioapic_write(ioapics[ioapic_id].virt_base, low_index, reg.low);
    ioapic_write(ioapics[ioapic_id].virt_base, high_index, reg.high);
}

void ioapic_add(const uint8_t id, const uintptr_t ioapic_base, const uint32_t int_base)
{
    assert(num_ioapics < MAX_IOAPICS);

    uintptr_t virt_base = (uintptr_t)ioapic_mapping + PAGE_SIZE * num_ioapics;

    free_frame(VMM_V2P(virt_base));
    map(virt_base, ioapic_base, 3);

    union ioapicver verreg = (union ioapicver)ioapic_read(virt_base, IOAPICVER);
    union ioapicid idreg = (union ioapicid)ioapic_read(virt_base, IOAPICID);
    assert(idreg.id == id);

    ioapics[num_ioapics].id = id;
    ioapics[num_ioapics].version = verreg.version;
    ioapics[num_ioapics].phys_base = ioapic_base;
    ioapics[num_ioapics].virt_base = virt_base;
    ioapics[num_ioapics].int_base = int_base;
    ioapics[num_ioapics].int_max = int_base + verreg.max_red_entry;

    for (int i = 0; i < verreg.max_red_entry + 1; ++i)
        ioapic_mask_irq(i, num_ioapics);

    printk(LOG " base: %#016x version: %u, %u redirection entries IOAPIC ID: %u\n", 
            ioapic_base, verreg.version, verreg.max_red_entry + 1, idreg.id);

    ++num_ioapics;
}

void ioapic_set_irq(const uint8_t irq, const uint64_t apic_id, const uint8_t vector)
{
    unsigned int i;
    for (i = 0; i < num_ioapics; ++i)
        if (irq >= ioapics[i].int_base && irq <= ioapics[i].int_max)
            break;
    if (i == num_ioapics) {
        printk("\33\x0f\x40<ioapic> IRQ #%u isn't mapped to the IOAPIC(s)\n", irq);
        return;
    }

    const uint32_t low_index  = IOREDTBL + irq * 2;
    const uint32_t high_index = IOREDTBL + irq * 2 + 1;

    union ioredtbl reg;
    reg.low = ioapic_read(ioapics[i].virt_base, low_index);
    reg.high = ioapic_read(ioapics[i].virt_base, high_index);

    reg.destination = apic_id;
    reg.mask = 0;
    reg.destination_mode = PHYSICAL_MODE;
    reg.delivery_mode = FIXED;
    reg.vector = vector;

    ioapic_write(ioapics[i].virt_base, low_index, reg.low);
    ioapic_write(ioapics[i].virt_base, high_index, reg.high);
}

