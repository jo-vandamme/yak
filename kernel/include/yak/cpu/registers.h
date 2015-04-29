#ifndef __YAK_REGISTERS_H__
#define __YAK_REGISTERS_H__

#include <yak/kernel.h>

typedef struct registers 
{
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rax;
    uint64_t vector;
    uint64_t error;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __packed registers_t;

void print_regs(registers_t *regs);

#endif

