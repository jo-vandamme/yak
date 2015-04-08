#ifndef __YAK_IDT_H__
#define __YAK_IDT_H__

#include <yak/lib/types.h>

#define IDT_NUM_ENTRIES 256

typedef struct
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idt_ptr_t;

void isr_init(void);
void idt_init(void);

#endif
