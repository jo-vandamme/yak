#include <yak/config.h>
#include <yak/cpu/mp.h>

.extern ap_main

.section .init.text

.globl trampoline
.globl trampoline_end

#-----------------------------------------
.code16
trampoline:
    cli
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss

    # Tell the BSP we have started
    movb $AP_STARTED, AP_STATUS_FLAG

    # wait for his acknowledgment before continuing
1:  movb AP_STATUS_FLAG, %al
    cmpb $AP_CONTINUE, %al
    jne 1b

    lidt (TRAMPOLINE_START + (idt_ptr - trampoline))
    lgdt (TRAMPOLINE_START + (gdt_ptr - trampoline))

    # Set PE bit to enable protected mode
    movl %cr0, %eax
    orl $1, %eax
    movl %eax, %cr0

    ljmp $0x08, $(TRAMPOLINE_START + (trampoline_32 - trampoline))

#-----------------------------------------
.code32
trampoline_32:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    # Set CR3 to PML4
    movl (TRAMPOLINE_PARAMS + CR3_OFFSET), %eax
    movl %eax, %cr3

    # Set CR4 PAE and PGE
    movl %cr4, %eax
    btsl $5, %eax        # PAE bit
    btsl $7, %eax        # PGE bit
    movl %eax, %cr4

    # Set LM bit of EFER MSR
    movl $0xc0000080, %ecx  # EFER MSR register
    rdmsr
    btsl $8, %eax        # LME bit
    wrmsr

    # Set CR0 PG bit to enable paging
    movl %cr0, %eax
    btsl $31, %eax       # PG bit
    movl %eax, %cr0

    lgdt (TRAMPOLINE_START + (gdt_ptr_64 - trampoline))
    ljmp $0x08, $(TRAMPOLINE_START + (trampoline_64 - trampoline))

#-----------------------------------------
.code64
trampoline_64:
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    cld

    lidt (TRAMPOLINE_PARAMS + IDT_OFFSET)
    lgdt (TRAMPOLINE_PARAMS + GDT_OFFSET)

    movq (TRAMPOLINE_PARAMS + STACK_OFFSET), %rsp

    # arguments to ap_main(id, percpu_base)
    movq (TRAMPOLINE_PARAMS + ID_OFFSET), %rdi
    movq (TRAMPOLINE_PARAMS + PERCPU_OFFSET), %rsi

    movq $ap_main, %rax
    jmpq *%rax

    cli
    hlt
spin:
    jmp spin

#-----------------------------------------
.align 16
gdt:
    .quad 0x0
    # code
    .word 0xffff
    .word 0x0000
    .word 0x9a00
    .word 0x00cf
    # data
    .word 0xffff
    .word 0x0000
    .word 0x9200
    .word 0x00cf
gdt_ptr:
    .word . - gdt - 1
    .long TRAMPOLINE_START + (gdt - trampoline)

.align 16
gdt_64:
    .quad 0x0
    # code
    .word 0xffff    # limit
    .word 0x0000    # base
    .word 0x9a00    # P=1, DPL=00, type=0xa (exec/read)
    .word 0x00af    # G=1 (4K), D=1, limit[16:19]=0xf
    # data
    .word 0xffff    # limit
    .word 0x0000    # base
    .word 0x9200    # P=1, DPL=00, type=0x2 (read/write)
    .word 0x008f    # G=1 (4K), B=1, limit[16:19] = 0xf
gdt_ptr_64:
    .word . - gdt_64 - 1
    .quad TRAMPOLINE_START + (gdt_64 - trampoline)

idt_ptr:
    .word 0 # zero limit, force shutdown if NMI
    .long 0 # base, ignored

trampoline_end:
