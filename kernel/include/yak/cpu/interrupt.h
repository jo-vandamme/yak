#ifndef __YAK_INTERRUPT_H__
#define __YAK_INTERRUPT_H__

#include <yak/kernel.h>
#include <yak/cpu/registers.h>

#define IRQ(x) ((x) + 0x20)

typedef void (*isr_t)(void *regs);

typedef struct isr_node isr_node_t;

struct isr_node
{
    isr_t isr;
    isr_node_t *next;
};

void isr_handlers_init(void);
void isr_register(const uint8_t vector, const isr_t isr);
void isr_unregister(const uint8_t vector, const isr_t isr);
isr_node_t *isr_get_list(unsigned int vector);

#endif
