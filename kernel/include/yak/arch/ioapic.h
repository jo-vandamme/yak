#ifndef __YAK_IOAPIC_H__
#define __YAK_IOAPIC_H__

#include <yak/lib/types.h>

// destination modes
#define DESTMOD_PHYSICAL        0
#define DESTMOD_LOGICAL         1

// delivery mode
#define DELMOD_FIXED            0
#define DELMOD_LOWEST_PRIORITY  1
#define DELMOD_SMI              2
#define DELMOD_NMI              4
#define DELMOD_INIT             5
#define DELMOD_EXTINT           7

// trigger mode
#define TRIGMOD_EDGE            0
#define TRIGMOD_LEVEL           1

// interrupt input pin polarity
#define INTPOL_HIGH             0
#define INTPOL_LOW              1

// delivery status (read-only)
#define DELIVS_IDLE             0
#define DELIVS_SEND_PENDING     1

typedef struct {
    uint32_t vector             : 8;
    uint32_t destination        : 8;
    uint32_t destination_mode   : 1;
    uint32_t delivery_mode      : 3;
    uint32_t mask               : 1;
} redtbl_entry_t;

void ioapic_add(const uint8_t id, const uintptr_t ioapic_base, const uint32_t int_base);
void ioapic_add_override(const uint8_t source, const uint32_t gsi, const uint16_t flags);

void ioapic_modify_irq(const uint8_t irq, const redtbl_entry_t entry);
void ioapic_set_irq(const uint8_t irq, const uint8_t vector, const uint64_t apic_id);

#endif
