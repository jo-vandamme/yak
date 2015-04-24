#ifndef __YAK_REGISTERS_H__
#define __YAK_REGISTERS_H__

#include <yak/kernel.h>

typedef struct registers 
{
    u64_t r11;
    u64_t r10;
    u64_t r9;
    u64_t r8;
    u64_t rsi;
    u64_t rdi;
    u64_t rdx;
    u64_t rcx;
    u64_t rax;
    u64_t int_no;
    u64_t error;
    u64_t rip;
    u64_t cs;
    u64_t rflags;
    u64_t rsp;
    u64_t ss;
} __packed registers_t;

void print_regs(registers_t *regs);

#endif

