#ifndef __CPUID_H__
#define __CPUID_H__

int has_cpuid(void)
{
    int result = 0;
    asm volatile(
            "pushfl\n"
            "popl %%eax\n"
            "movl %%eax, %%ecx\n"
            "xorl $(1 << 21), %%eax\n" // flip bit 21
            "pushl %%eax\n"
            "popfl\n"
            "pushfl\n"
            "popl %%eax\n"
            "pushl %%ecx\n"
            "popfl\n"
            "xorl %%eax, %%ecx\n"
            "jz no_cpuid\n"
            "movl $1, %%eax\n"
            "jmp 1f\n"
            "no_cpuid: movl $0, %%eax\n"
            "1:" : "=a"(result));

    return result;
}

int has_longmode(void)
{
    int result = 0;
    asm volatile(
            "movl $0x80000000, %%eax\n"
            "cpuid\n"
            "cmpl $0x80000001, %%eax\n"
            "jb no_longmode\n"
            "movl $0x80000001, %%eax\n"
            "cpuid\n"
            "testl $(1 << 29), %%edx\n"
            "jz no_longmode\n"
            "movl $1, %%eax\n"
            "jmp 1f\n"
            "no_longmode: movl $0, %%eax\n"
            "1:" : "=a"(result));
    return result;
}

#endif
