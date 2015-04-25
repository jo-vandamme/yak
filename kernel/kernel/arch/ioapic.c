#include <yak/kernel.h>
#include <yak/mem/pmm.h>
#include <yak/mem/vmm.h>
#include <yak/arch/spinlock.h>
#include <yak/arch/ioapic.h>

#define LOG LOG_COLOR0 "ioapic:\33r"

#define MAX_IOAPICS 8 // arbitrary

#define IOREGSEL    0x00
#define IOREGWIN    0x10

#define IOAPICID    0x00
#define IOAPICVER   0x01
#define IOAPICARB   0x02
#define IOREDTBL    0x10

static spinlock_t lock;

char ioapic_mapping[PAGE_SIZE * MAX_IOAPICS] __attribute__((aligned(PAGE_SIZE)));

typedef struct ioapic
{
    uint8_t id;
    uintptr_t phys_base;
    uintptr_t virt_base;
    uint32_t int_base;
    uint8_t int_max;
} ioapic_t;

static unsigned int num_ioapics = 0;
static ioapic_t ioapics[MAX_IOAPICS]; 

typedef struct
{
    unsigned int gsi;
    struct {
        unsigned int source : 8;
        unsigned int flags  : 16;
    };
} __packed int_override_t;

#define MAX_INT_OVERRIDES 20 // arbitrary

static unsigned int num_int_overrides = 0;
static int_override_t int_overrides[MAX_INT_OVERRIDES];

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

union ioredtbl {
    struct {
        uint64_t vector             : 8;
        uint64_t delivery_mode      : 3;
        uint64_t destination_mode   : 1;
        uint64_t delivery_status    : 1; // read-only
        uint64_t input_pin_polarity : 1;
        uint64_t remote_irr         : 1; // read-only
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

    spin_lock(&lock);

    union ioredtbl reg;
    reg.low = ioapic_read(ioapics[ioapic_id].virt_base, low_index);
    reg.high = ioapic_read(ioapics[ioapic_id].virt_base, high_index);
    reg.mask = 1;
    ioapic_write(ioapics[ioapic_id].virt_base, low_index, reg.low);
    ioapic_write(ioapics[ioapic_id].virt_base, high_index, reg.high);

    spin_unlock(&lock);
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

#define MPS_INTI_ACTIVE_HIGH    1
#define MPS_INTI_ACTIVE_LOW     3
#define MPS_INTI_TRIGGER_EDGE   1
#define MPS_INTI_TRIGGER_LEVEL  3
#define MPS_INTI_CONFORM        0

void ioapic_modify_irq(const uint8_t irq, const redtbl_entry_t entry)
{
    spin_lock(&lock);

    // check if the irq has been rerouted
    uint8_t gsi = irq, override_found = 0;
    uint16_t flags;
    for (int i = 0; i < MAX_INT_OVERRIDES; ++i) {
        if (int_overrides[i].source == irq) {
            gsi = int_overrides[i].gsi;
            flags = int_overrides[i].flags;
            override_found = 1;
            break;
        }
    }
    
    // search the ioapic on which the irq is routed
    unsigned int id;
    for (id = 0; id < num_ioapics; ++id)
        if (gsi >= ioapics[id].int_base && gsi <= ioapics[id].int_max)
            break;
    if (id == num_ioapics) {
        printk("\33\x0f\x40<ioapic> IRQ #%u isn't mapped to the IOAPIC(s)\n", gsi);
        return;
    }
    printk(LOG " mapping irq #%u (gsi #%u, ioapic #%u) to isr #%u on lapic #%u\n", 
            irq, gsi, id, entry.vector, entry.destination);

    const uint32_t low_index  = IOREDTBL + gsi * 2;
    const uint32_t high_index = IOREDTBL + gsi * 2 + 1;

    union ioredtbl reg;
    reg.low = ioapic_read(ioapics[id].virt_base, low_index);
    reg.high = ioapic_read(ioapics[id].virt_base, high_index);

    // if there is an interrupt override, use its flags
    if (override_found) {
        uint8_t pol = flags & 0x03;
        uint8_t trig = (flags >> 2) & 0x03;

        if (pol == MPS_INTI_CONFORM) ;
            // conforms to the specification of the bus
        else if (pol == MPS_INTI_ACTIVE_HIGH)
            reg.input_pin_polarity = INTPOL_HIGH;
        else if (pol == MPS_INTI_ACTIVE_LOW)
            reg.input_pin_polarity = INTPOL_LOW;

        if (trig == MPS_INTI_CONFORM) ;
            // conforms to the specification of the bus
        else if (trig == MPS_INTI_TRIGGER_EDGE)
            reg.trigger_mode = TRIGMOD_EDGE;
        else if (trig == MPS_INTI_TRIGGER_LEVEL)
            reg.trigger_mode = TRIGMOD_LEVEL;
    }

    reg.vector = entry.vector;
    reg.delivery_mode = entry.delivery_mode;
    reg.destination_mode = entry.destination_mode;
    reg.mask = entry.mask;
    reg.destination = entry.destination;

    ioapic_write(ioapics[id].virt_base, low_index, reg.low);
    ioapic_write(ioapics[id].virt_base, high_index, reg.high);

    spin_unlock(&lock);
}

void ioapic_set_irq(const uint8_t irq, const uint8_t vector, const uint64_t apic_id)
{
    redtbl_entry_t entry = {
        .vector = vector,
        .delivery_mode = DELMOD_FIXED,
        .destination_mode = DESTMOD_PHYSICAL,
        .mask = 0,
        .destination = apic_id
    };
    ioapic_modify_irq(irq, entry);
}

void ioapic_add_override(const uint8_t source, const uint32_t gsi, const uint16_t flags)
{
    assert(num_int_overrides < MAX_INT_OVERRIDES);

    int_overrides[num_int_overrides].source = source;
    int_overrides[num_int_overrides].gsi = gsi;
    int_overrides[num_int_overrides].flags = flags;

    printk(LOG " interrupt override: %u->%u, flags = %#04x\n", source, gsi, flags);

    ++num_int_overrides;
}
