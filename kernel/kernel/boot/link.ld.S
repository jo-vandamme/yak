#include <yak/config.h>

OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(entry)

SECTIONS
{
    . = PHYSICAL_BASE;

    .image_start ALIGN(PAGE_SIZE) : AT(ADDR(.image_start) - VIRTUAL_BASE)
    {
        image_start = .;
    }

    boot_mem_start = . + VIRTUAL_BASE;

    .multiboot : {
        . = ALIGN(16);
        *(.multiboot)
    }

    .bootstrap ALIGN(PAGE_SIZE) : {
        *(.bootstrap)
        . = ALIGN(PAGE_SIZE);
    }

    boot_mem_end = . + VIRTUAL_BASE;

    . += VIRTUAL_BASE;

    init_mem_start = .;

    .init.text : AT(ADDR(.init.text) - VIRTUAL_BASE)
    {
        *(.init.text)
    }

    .init.data : AT(ADDR(.init.data) - VIRTUAL_BASE)
    {
        *(.init.data)
    }
    
    .init.bss : AT(ADDR(.init.bss) - VIRTUAL_BASE)
    {
        *(.init.bss)
    }

    init_mem_end = .;

    .kstack : AT(ADDR(.kstack) - VIRTUAL_BASE)
    {
        *(.kstack)
    }

    .text ALIGN(PAGE_SIZE) : AT(ADDR(.text) - VIRTUAL_BASE)
    {
        *(.text)
        *(.rodata*)
    }

    .data ALIGN(PAGE_SIZE) : AT(ADDR(.data) - VIRTUAL_BASE)
    {
        *(.data)
    }

    .bss ALIGN(PAGE_SIZE) : AT(ADDR(.bss) - VIRTUAL_BASE)
    {
        kernel_bss_start = .;
        *(COMMON)
        *(.bss)
        kernel_bss_end = .;
    }

    .data.percpu ALIGN(PAGE_SIZE) : AT(ADDR(.data.percpu) - VIRTUAL_BASE)
    {
        kernel_percpu_start = .;
        *(.data.percpu.header)
        *(.data.percpu)
        kernel_percpu_end = .;
        . = ALIGN(PAGE_SIZE);
    }

    kernel_end = .;

    /DISCARD/ : 
    {
        *(.eh_frame)
        *(.comment)
        *(.note)
    }
}
