#include <yak/config.h>
#include <yak/initcall.h>
#include <yak/video/printk.h>
#include <yak/arch/lapic.h>
#include <yak/cpu/idt.h>
#include <yak/cpu/registers.h>
#include <yak/cpu/interrupt.h>

#define LOG "\33\x0a\xf0<isr>\33r"
#define ISR_POOL_SIZE 256

typedef struct isr_node isr_node_t;

struct isr_node
{
    isr_t isr;
    isr_node_t *next;
};

static isr_node_t *isr_lists[IDT_NUM_ENTRIES];
static isr_node_t isr_pool[ISR_POOL_SIZE];
static isr_node_t *isr_stack;

static inline isr_node_t *alloc_node(void)
{
    isr_node_t *node = isr_stack;
    if (!node)
        panic(LOG "ISR pool is full\n");
    isr_stack = node->next;
    return node;
}

static inline void free_node(isr_node_t *node)
{
    if (node) {
        node->next = isr_stack;
        isr_stack = node;
    }
}

void isr_register(const uint8_t vector, const isr_t isr)
{
    isr_node_t *new_node = alloc_node();
    new_node->isr = isr;
    new_node->next = 0;

    isr_node_t *node = isr_lists[vector];
    if (!node) {
        isr_lists[vector] = new_node;
        return;
    }
    while (node->next)
        node = node->next;
    node->next = new_node;
}

void isr_unregister(const uint8_t vector, const isr_t isr)
{
    isr_node_t *node = isr_lists[vector];
    isr_node_t *prev = node;
    do {
        if (node->isr == isr) {
            prev->next = node->next;
            free_node(node);
            break;
        }
        prev = node;
        node = node->next;
    } while (node);
}

INIT_CODE void isr_handlers_init(void)
{
    isr_stack = &isr_pool[0];
    isr_node_t *node = isr_stack;
    for (unsigned int i = 0; i < ISR_POOL_SIZE; ++i) {
        node->next = &isr_pool[i];
        node = node->next;
    }
}

static char *exception_messages[];

void fault_handler(registers_t *regs)
{
    if (regs->int_no == 14) {
        uintptr_t faulting_address;
        asm volatile("movq %%cr2, %0" : "=r"(faulting_address));
        printk("\n\33\x0f\x10page fault @ %08x%08x", faulting_address >> 32, faulting_address);
    }
    panic("Exception #%u: %s\nError code: 0x%x\n" \
          "rip: 0x%08x%08x\nrsp: 0x%08x%08x\n" \
          "rfl: 0x%08x%08x\ncs: 0x%04x ss: 0x%04x\n",
          regs->int_no,
          exception_messages[regs->int_no] ? 
          exception_messages[regs->int_no] : "Unknown", regs->error,
          regs->rip >> 32, regs->rip, 
          regs->rsp >> 32, regs->rsp,
          regs->rflags >> 32, regs->rflags, 
          regs->cs, regs->ss);
}

void interrupt_dispatch(void *r)
{
    registers_t *regs = (registers_t *)r;
    if (regs->int_no < IRQ(0)) {
        fault_handler(regs);
    }

    // call the list of handlers
    isr_node_t *node = isr_lists[regs->int_no];
    while (node) {
        if (node->isr)
            node->isr(regs);
        node = node->next;
    }

    lapic_ack_irq();

    //printk("interrupt #%u lapic #%u\n", regs->int_no, lapic_id());
}

static char *exception_messages[] = {
    [ 0] = "Divide-by-zero Error",
    [ 1] = "Debug",
    [ 2] = "Non-maskable Interrupt",
    [ 3] = "Breakpoint",
    [ 4] = "Overflow",
    [ 5] = "Bound Range Exceeded",
    [ 6] = "Invalid Opcode",
    [ 7] = "Device not available",
    [ 8] = "Double fault",
    [ 9] = "Coprocessor Segment Overrun",
    [10] = "Invalid TSS",
    [11] = "Segment Not Present",
    [12] = "Stack-Segment Fault",
    [13] = "General Protection Fault",
    [14] = "Page Fault",
    [16] = "x87 Floating-Point Exception",
    [17] = "Alignment Check",
    [18] = "Machine Check",
    [19] = "SIMD Floating-Point Exception",
    [20] = "Virtualization Exception",
    [30] = "Security Exception"
};
