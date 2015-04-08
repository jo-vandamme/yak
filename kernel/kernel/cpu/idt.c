#include <yak/initcall.h>
#include <yak/lib/types.h>
//#include <yak/video/printk.h>
#include <yak/cpu/interrupt.h>
#include <yak/cpu/gdt.h>
#include <yak/cpu/idt.h>

// see Intel SDM figure 6-7
typedef struct
{
    uint16_t base_low;
    uint16_t selector;
    uint16_t flags;
    uint16_t base_mid;
    uint32_t base_high;
    uint32_t zero;
} __attribute__((packed)) idt_entry_t;

// this table is shared by all cpus, it is defined in isr.S
extern idt_entry_t idt[IDT_NUM_ENTRIES];

#define IDT_PRESENT      (0x01 << 15)
#define IDT_TYPE_32_TRAP (0x0f <<  8)
#define IDT_RING0        (0x00 << 13)
#define IDT_RING3        (0x03 << 13)

void idt_set_gate(idt_entry_t *gate, uint64_t isr, uint16_t flags, uint16_t sel)
{
    gate->base_low = isr & 0xffff;
    gate->base_mid = (isr >> 16) & 0xffff;
    gate->base_high = (isr >> 32) & 0xffffffff;
    gate->selector = sel;
    gate->flags = flags;
    gate->zero = 0;
}

INIT_CODE void idt_init(void)
{
    volatile idt_ptr_t idt_ptr;
    idt_ptr.base = (uint64_t)idt;
    idt_ptr.limit = sizeof(idt_entry_t) * IDT_NUM_ENTRIES - 1;

    asm volatile("lidt (%0)" : : "r"(&idt_ptr));

    //printk("\33\x0a\xf0<idt>\33r idt installed\n");
}

INIT_CODE void isr_init(void)
{
    for (int i = 0; i < IDT_NUM_ENTRIES; ++i) {
        uint64_t isr = *((uint64_t *)idt + i * 2);
        idt_set_gate(&idt[i], isr, IDT_PRESENT | IDT_TYPE_32_TRAP, SEG_KCODE << 3);
    }
    idt[128].flags |= IDT_RING3;

    isr_handlers_init();

    //printk("\33\x0a\xf0<isr>\33r interrupt service routines installed\n");
}

