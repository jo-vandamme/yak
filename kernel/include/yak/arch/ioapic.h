#ifndef __YAK_IOAPIC_H__
#define __YAK_IOAPIC_H__

#include <yak/lib/types.h>

void ioapic_add(const uint8_t id, const uintptr_t ioapic_base, const uint32_t int_base);

void ioapic_set_irq(const uint8_t irq, const uint64_t apic_id, const uint8_t vector);

#endif
