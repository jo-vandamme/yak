#ifndef __YAK_LAPIC_H__
#define __YAK_LAPIC_H__

#include <yak/lib/types.h>

void lapic_init(const uintptr_t lapic_base);
void lapic_enable(const int bsp);
uint8_t lapic_id(void);
uint8_t lapic_version(void);
void lapic_clear_error(void);
void lapic_ack_irq(void);

void lapic_send_ipi(const uint8_t lapic_id, const uint8_t vector);
void lapic_send_startup_ipi(const uint8_t lapic_id, const uintptr_t addr);
void lapic_send_init_ipi(const uint8_t lapic_id);

#endif
