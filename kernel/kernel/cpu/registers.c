#include <yak/kernel.h>
#include <yak/cpu/registers.h>

void print_regs(registers_t *regs)
{
    printk("cs: %#04x ss: %#04x\n" \
           "rip: %#016x rsp: %#016x\n" \
           "rfl: %#016x rax: %#016x\n" \
           "rcx: %#016x rdx: %#016x\n" \
           "rsi: %#016x rdi: %#016x\n",
           regs->cs, regs->ss,
           regs->rip, regs->rsp, regs->rflags, 
           regs->rax, regs->rcx, regs->rdx,
           regs->rsi, regs->rdi);
}

