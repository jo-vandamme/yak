#include <yak/kernel.h>
#include <yak/initcall.h>
#include <yak/lib/pool.h>
#include <yak/arch/lapic.h>
#include <yak/cpu/idt.h>
#include <yak/cpu/registers.h>
#include <yak/cpu/interrupt.h>

#define LOG "\33\x0a\xf0<isr>\33r"
#define ISR_POOL_SIZE 256

POOL_DECLARE(isr_pool, struct isr_node, ISR_POOL_SIZE);

static isr_node_t *isr_lists[IDT_NUM_ENTRIES];

INIT_CODE void isr_handlers_init(void)
{
    POOL_INIT(isr_pool);
}

void isr_register(const uint8_t vector, const isr_t isr)
{
    isr_node_t *new_node = POOL_ALLOC(isr_pool);
    new_node->isr = isr;
    new_node->next = 0;

    isr_node_t *node = isr_lists[vector];
    if (!node) {
        isr_lists[vector] = new_node;
        return;
    }
    // append the new node at the end
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
            POOL_FREE(isr_pool, node);
            break;
        }
        prev = node;
        node = node->next;
    } while (node);
}

inline isr_node_t *isr_get_list(unsigned int vector)
{
    return isr_lists[vector];
}

union page_fault_error
{
    uint32_t raw;
    struct {
        uint32_t present           : 1;
        uint32_t write             : 1;
        uint32_t user              : 1;
        uint32_t                   : 1;
        uint32_t instruction_fetch : 1;
        uint32_t                   : 27;
    };
};

static int page_fault_handler(__unused registers_t *regs)
{
    union page_fault_error error = { .raw = regs->error };
    uintptr_t faulting_address;
    asm volatile("movq %%cr2, %0" : "=r"(faulting_address));

    printk("\n\33\x0f\x10Page fault @ %016x %s %s %s %s\n", faulting_address,
           error.present ? "[Present]" : "",
           error.write ? "[Write]" : "",
           error.user ? "[User]" : "",
           error.instruction_fetch ? "[Instruction fetch]" : "");

    return 1;
}

static char *exception_messages[];

void interrupt_dispatch(void *r)
{
    int stop = 0;
    registers_t *regs = (registers_t *)r;
    isr_node_t *node = isr_get_list(regs->vector);

    // handle exceptions
    if (regs->vector < IRQ(0)) {
        stop = 1;
        switch (regs->vector) {
            case 14:
                if (page_fault_handler(regs) == 0)
                    stop = 0;
                break;
            default:
                break;
        }
    }
    if (!stop && !node)
        printk("Uncaught exception %u\n", regs->vector);

    // handlers execution for exceptions and irq
    while (node) {
        if (node->isr)
            node->isr(regs);
        node = node->next;
    }

    if (stop) {
        printk("Exception #%u: %s\nError code: %#x\n", regs->vector, 
            exception_messages[regs->vector] ? 
            exception_messages[regs->vector] : "Unknown", regs->error);
        print_regs(regs);
        panic("Aborting\n");
    }

    lapic_ack_irq();

    //printk("interrupt #%u lapic #%u\n", regs->vector, lapic_id());
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
