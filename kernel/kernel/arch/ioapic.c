#include <yak/kernel.h>
#include <yak/mem/vmm.h>
#include <yak/arch/ioapic.h>

#define LOG "\33\x0a\xf0<ioapic>\33r"

#define MAX_IOAPICS 8 // arbitrary

#define IOREGSEL    0x00
#define IOREGWIN    0x10

#define IOAPICID    0x00
#define IOAPICVER   0x01
#define IOAPICARB   0x02
#define IOREDTBL    0x10

typedef struct ioapic
{
    uint8_t id;
    uint8_t version;
    uint8_t max_irq;
    uintptr_t base;
} ioapic_t;

static unsigned int num_ioapics = 0;
static ioapic_t ioapics[MAX_IOAPICS]; 

void ioapic_write(const uintptr_t ioapic_base, const uint8_t offset, const uint32_t val)
{
    *(uint32_t *)(ioapic_base + IOREGSEL) = offset;
    mem_barrier();
    *(uint32_t *)(ioapic_base + IOREGWIN) = val;
    mem_barrier();
}

uint32_t ioapic_read(const uintptr_t ioapic_base, const uint8_t offset)
{
    *(uint32_t *)(ioapic_base + IOREGSEL) = offset;
    mem_barrier();
    uint32_t val = *(uint32_t *)(ioapic_base + IOREGWIN);
    mem_barrier();
    return val;
}

void ioapic_add(const uint8_t id, const uintptr_t ioapic_base)
{
    printk(LOG " id: %u base: 0x%08x%08x\n", id, ioapic_base >> 32, ioapic_base);
    assert(num_ioapics < MAX_IOAPICS);
    ioapics[num_ioapics].id = id;
    ioapics[num_ioapics].base = ioapic_base;
    ++num_ioapics;
    map(ioapic_base, ioapic_base, 3);
}

void ioapic_set_irq(const uint8_t irq, const uint64_t apic_id, const uint8_t vector)
{
    const uint32_t low_index  = IOREDTBL + irq * 2;
    const uint32_t high_index = IOREDTBL + irq * 2 + 1;

    uint32_t low = ioapic_read(ioapics[0].base, low_index);
    uint32_t high = ioapic_read(ioapics[0].base, high_index);

    // set the apic id (bits 56-59 for physical destination)
    high &= ~0xff000000;
    high |= apic_id << 24;
    ioapic_write(ioapics[0].base, high_index, high);

    // unmask the irq (bit 16 = 0)
    low &= ~(1 << 16);

    // set the to physical delivery mode (bit 11 = 0)
    low &= ~(1 << 11);

    // set to fixed delivery mode (bits 8-10 = 000)
    low &= ~0x700;

    // set the delivery vector (bits 0-7 = vector)
    low &= ~0xff;
    low |= vector;
    ioapic_write(ioapics[0].base, low_index, low);
}

void ioapic_init(void)
{
    uint32_t ver = ioapic_read(ioapics[0].base, IOAPICVER);
    unsigned int count = ((ver >> 16) & 0xff) + 1;
    printk(LOG " max redirection entries: %u\n", count);

    local_irq_enable();
}
