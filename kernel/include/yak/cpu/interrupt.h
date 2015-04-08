#ifndef __YAK_INTERRUPT_H__
#define __YAK_INTERRUPT_H__

#include <yak/cpu/registers.h>

#define IRQ(x) ((x) + 0x20)

typedef void (*isr_t)(registers_t *regs);

void isr_register(const uint8_t vector, const isr_t isr);
void isr_unregister(const uint8_t vector, const isr_t isr);
void isr_handlers_init(void);

#endif
