#include <yak/kernel.h>
#include <yak/cpu/registers.h>

void print_regs(registers_t *regs)
{
    printk("rip: %#016x\nrsp: %#016x\n" \
           "rfl: %#016x\ncs: %#04x ss: %#04x\n",
           regs->rip, regs->rsp, regs->rflags, regs->cs, regs->ss);
}

