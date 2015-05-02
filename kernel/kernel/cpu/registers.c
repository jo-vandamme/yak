#include <yak/kernel.h>
#include <yak/cpu/registers.h>

struct stack_frame
{
    struct stack_frame *prev;
    uintptr_t return_address;
} __packed;

void backtrace_from_fp(uintptr_t *buf, int size)
{
    struct stack_frame *fp;

    asm volatile("movq %%rbp, %[fp]" : [fp]"=r"(fp));

    printk("\t#  saved fp           return address\n");
    for (int i = 0; i < size && fp != 0; fp = fp->prev, ++i) {
        printk("\t%02u %#016x %#016x\n", i, fp, fp->return_address);
        buf[i] = fp->return_address;
    }
}

void print_regs(registers_t *regs)
{
    printk("\tcs: %#04x ss: %#04x\n" \
           "\trip: %#016x rsp: %#016x\n" \
           "\trfl: %#016x rax: %#016x\n" \
           "\trcx: %#016x rdx: %#016x\n" \
           "\trsi: %#016x rdi: %#016x\n",
           regs->cs, regs->ss,
           regs->rip, regs->rsp, regs->rflags, 
           regs->rax, regs->rcx, regs->rdx,
           regs->rsi, regs->rdi);

    printk("\n\tStack trace:\n");
    const int size = 20;
    uintptr_t addresses[size];
    backtrace_from_fp(addresses, size);

    //for (int i = 0; i < size; ++i)
        //printk("\t%#016x\n", addresses[i]);
}

