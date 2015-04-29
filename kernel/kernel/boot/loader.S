#include <yak/config.h>

.extern kernel_main

#-----------------------------------------
.section .multiboot, "a"

.set PAGEALIGN, 1 << 0
.set MEMINFO,   1 << 1
.set VIDEO,     1 << 2
.set FLAGS,     PAGEALIGN | MEMINFO | VIDEO
.set MAGIC,     0x1badb002
.set CHECKSUM, -(MAGIC + FLAGS)

.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM
.long 0, 0, 0, 0, 0
.long 0
.long 1024
.long 768
.long 32

#-----------------------------------------
.section .kstack, "aw", @nobits

.align 128
stack_begin:
.skip STACK_SIZE
stack_end:

#-----------------------------------------
.section .bss, "aw", @nobits

.align PAGE_SIZE
PML4:
.skip PAGE_SIZE
kernel_PDPT:
.skip PAGE_SIZE
kernel_PDT:
.skip PAGE_SIZE
kernel_PT:
.skip PAGE_SIZE
fb_PDPT:
.skip PAGE_SIZE
fb_PDT:
.skip PAGE_SIZE

#-----------------------------------------
# init sections will be reclaimed
# once the kernel will be up and running
.section .init.bss, "aw", @nobits

.align PAGE_SIZE
id_PDPT:
.skip PAGE_SIZE
id_PDT:
.skip PAGE_SIZE

#-----------------------------------------
.section .bootstrap, "ax"
.code32

.globl entry
.type entry, @function
entry:
    cld
    cli

    # push multiboot magic (64-bit)
    pushl $0
    pushl %eax
    # push multiboot info pointer (64-bit)
    pushl $0
    pushl %ebx

    # Identity map the first 2MB
    movl $(id_PDPT - VIRTUAL_BASE + 3), (PML4 - VIRTUAL_BASE)
    movl $(id_PDT - VIRTUAL_BASE + 3), (id_PDPT - VIRTUAL_BASE)
    movl $0x83, (id_PDT - VIRTUAL_BASE)

    # Map 1st 2MB to VIRTUAL_BASE
    movl $(kernel_PDPT - VIRTUAL_BASE + 3), (PML4 - VIRTUAL_BASE + 511 * 8)
    movl $(kernel_PDT - VIRTUAL_BASE + 3), (kernel_PDPT - VIRTUAL_BASE + 510 * 8)
    movl $(kernel_PT - VIRTUAL_BASE + 3), (kernel_PDT - VIRTUAL_BASE)

    movl $3, %ebx
    movl $512, %ecx
    movl $(kernel_PT - VIRTUAL_BASE), %edi
1:
    movl %ebx, (%edi)
    addl $0x1000, %ebx
    addl $8, %edi
    loop 1b

    # framebuffer mapping (0xfeff'c000'0000 = 0xffff'feff'c000'0000)
    movl $(fb_PDPT - VIRTUAL_BASE + 3), (PML4 - VIRTUAL_BASE + 509 * 8)
    movl $(fb_PDT - VIRTUAL_BASE + 3), (fb_PDPT - VIRTUAL_BASE + 511 * 8)
    movl (%esp), %eax # get the multiboot info pointer
    movl 76(%eax), %eax # get the vbe_mode_info field from the multiboot info struct
    movl 40(%eax), %eax # get the phys_base field from the vbe mode info struct
    shrl $12, %eax # align to page boundary
    shll $12, %eax

    addl $((1 << 7) + 3), %eax # Page size bit for 2MB pages
    movl $512, %ecx # map 512*2MB = 1GB
    movl $(fb_PDT - VIRTUAL_BASE), %edi
1:
    movl %eax, (%edi)
    addl $0x200000, %eax
    addl $8, %edi
    loop 1b

    # Recursive mapping via 510th entry
    movl $(PML4 - VIRTUAL_BASE + 3), (PML4 - VIRTUAL_BASE + 510 * 8)

    # Set CR3 to PML4
    movl $(PML4 - VIRTUAL_BASE), %edi
    movl %edi, %cr3
    
    # Set CR4 PAE
    movl %cr4, %eax
    btsl $5, %eax       # PAE bit
    btsl $7, %eax       # PGE bit
    movl %eax, %cr4

    # Set LM bit of EFER MSR
    movl $0xc0000080, %ecx  # EFER MSR register
    rdmsr
    btsl $8, %eax
    wrmsr

    # Set CR0 PG bit to enable paging, and WP bit to enable write protect
    movl %cr0, %eax
    btsl $16, %eax      # WP bit
    btsl $31, %eax      # PG bit
    movl %eax, %cr0

    # Enable SSE
    movl %cr0, %eax
    btcl $1, %eax       # clear CR0.EM bit
    btsl $0, %eax       # set CR0.MP bit
    movl %eax, %cr0
    movl %cr4, %eax
    btsl $8, %eax       # set CR4.OSFXSR bit
    btsl $9, %eax       # set CR4.OSXMMEXCPT bit
    movl %eax, %cr4

    # Enter the 64-bit submode by loading a 64-bit GDT 
    lgdt gdt_ptr
    ljmp $0x08, $longmode

.size entry, . - entry

#-----------------------------------------
.code64
.type longmode, @function
longmode:
    cli
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    cld

    popq %rsi
    popq %rdi

    movq $stack_end, %rsp

    # load a GDT located in high memory
    lgdt gdt_ptr_high

	movq $kernel_main, %rcx
    jmpq *%rcx

    cli
    hlt
.size longmode, . - longmode

#-----------------------------------------
# This GDT is located in low memory

.align 16
gdt:
    .quad 0x0
    #.set code, . - gdt
    .word 0xffff
    .word 0x0000
    .word 0x9a00
    .word 0x00af
    #.set data, . - gdt
    .word 0xffff
    .word 0x0000
    .word 0x9200
    .word 0x008f
gdt_ptr:
    .word . - gdt - 1
    .quad gdt

#-----------------------------------------
# This GDT is located in high memory

.section .data 

.align 16
gdt_high:
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
gdt_ptr_high:
    .word . - gdt_high - 1
    .quad gdt_high

