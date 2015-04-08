#include <yak/config.h>
#include <yak/initcall.h>
#include <yak/lib/types.h>
#include <yak/lib/string.h>
#include <yak/lib/utils.h>
#include <yak/boot/multiboot.h>
#include <yak/video/terminal.h>
#include <yak/video/vbe.h>
#include <yak/video/printk.h>
#include <yak/mem/pmm.h>
#include <yak/mem/vmm.h>
#include <yak/mem/mmap.h>
#include <yak/mem/mem.h>

#define logid "\33\x0a\xf0<mem>\33r"

extern multiboot_info_t *mbi;
extern vbe_mode_info_t *mode_info;

INIT_DATA static uintptr_t early_placement_address = 0;

/* make an early allocation that can't be deleted, the placement
 * address points right after the kernel and is incremented after
 * each allocation, the address given is virtual since a 4MB page
 * is mapped to high memory for the kernel and 4MB is sufficient for
 * both the kernel, its data and the early allocations
 */
static INIT_CODE uintptr_t alloc_early(size_t nbytes, size_t align)
{
    if (early_placement_address == 0) {
        early_placement_address = align_up((uintptr_t)kernel_end, PAGE_SIZE);
    }
    if (align > 1 && (early_placement_address % align) != 0) {
        early_placement_address += align - early_placement_address % align;
    }
    uintptr_t address = early_placement_address;
    early_placement_address += nbytes;

    return address;
}

/* relocate the structures allocated by the bootloader
 * and update the multiboot info pointer and fields
 */
INIT_CODE void relocate_structures(void)
{
    /* first relocate multiboot_info_t struct */
    uintptr_t ptr = alloc_early(sizeof(multiboot_info_t), 4);
    memcpy((void *)ptr, (void *)mbi, sizeof(multiboot_info_t));
    mbi = (multiboot_info_t *)ptr;

    if (mbi->flags & MBOOT_VBE) {
        /* relocate vbe_info_t struct */
        ptr = alloc_early(sizeof(vbe_info_t), 4);
        memcpy((void *)ptr, (void *)((uintptr_t)mbi->vbe_control_info), sizeof(vbe_info_t));
        mbi->vbe_control_info = VMM_V2P(ptr);

        /* relocate vbe_mode_info_t struct */
        ptr = alloc_early(sizeof(vbe_mode_info_t), 4);
        memcpy((void *)ptr, (void *)((uintptr_t)mbi->vbe_mode_info), sizeof(vbe_mode_info_t));
        mbi->vbe_mode_info = VMM_V2P(ptr);
    }

    if (mbi->flags & MBOOT_MMAP) {
        /* relocate the memory map */
        ptr = alloc_early(mbi->mmap_length, sizeof(mmap_entry_t));
        memcpy((void *)ptr, (void *)((uintptr_t)mbi->mmap_addr), mbi->mmap_length);
        mbi->mmap_addr = VMM_V2P(ptr);
    }

    if (mbi->flags & MBOOT_LOADER) {
        /* relocate bootloader name */
        ptr = alloc_early(strlen((const char *)((uintptr_t)mbi->boot_loader_name)), 4);
        strcpy((char *)ptr, (const char *)((uintptr_t)mbi->boot_loader_name));
        mbi->boot_loader_name = VMM_V2P(ptr);
    }
    printk("%s multiboot structures relocated\n", logid);
}

extern const char kernel_percpu_start[];
extern const char kernel_percpu_end[];

// This should be called before mem_init so the percpu areas are consecutive
// with the bsp percpu area
INIT_CODE void percpu_mem_init(int ncpus, uintptr_t *area, uintptr_t *stack)
{
    size_t percpu_size = (uintptr_t)kernel_percpu_end - (uintptr_t)kernel_percpu_start;

    *area = alloc_early(percpu_size * ncpus, PAGE_SIZE);
    //memset((void *)*area, 0, percpu_size * ncpus);

    *stack = alloc_early(STACK_SIZE * ncpus, PAGE_SIZE);
    memset((void *)*stack, 0, STACK_SIZE * ncpus);
}

INIT_CODE void mem_init(void)
{
    pmm_init((uintptr_t)image_start, (uintptr_t)early_placement_address - VIRTUAL_BASE);
    vmm_init();
}

extern const char boot_mem_start[];
extern const char init_mem_end[];

void reclaim_init_mem(void)
{
    uintptr_t start = align_up((uintptr_t)boot_mem_start, PAGE_SIZE);
    uintptr_t stop = align_down((uintptr_t)init_mem_end, PAGE_SIZE);

    unmap(start, (stop - start) / PAGE_SIZE);

    uintptr_t frame, n = 0;
    for (frame = start; frame < stop; frame += PAGE_SIZE, ++n) {
        free_frame(frame - VIRTUAL_BASE);
    }

    printk("%s reclaimed init memory %08x%08x:%08x%08x - %u frames\n", 
            logid, start >> 32, start, stop >> 32, stop, n);
}

/*
void print_image_sections(void)
{
    kprintf("\n\tsection      start:end    size (bytes)\n" \
            "\t--------------------------------------\n" \
            "\timage:       %08x:%08x -> %x\n" \
            "\tboot_mem:    %08x:%08x -> %x\n" \
            "\tinit_mem:    %08x:%08x -> %x\n" \
            "\tinit_code:   %08x:%08x -> %x\n" \
            "\tinit_data:   %08x:%08x -> %x\n" \
            "\tkstack:      %08x:%08x -> %x\n" \
            "\tkernel_code: %08x:%08x -> %x\n" \
            "\tkernel_data: %08x:%08x -> %x\n" \
            "\tkernel_bss:  %08x:%08x -> %x\n" \
            "\tpercpu:      %08x:%08x -> %x\n" \
            "\tearly alloc: %08x:%08x -> %x\n\n",
            image_start, image_end, image_end - image_start, 
            boot_mem_start, boot_mem_end, boot_mem_end - boot_mem_start,
            init_mem_start, init_mem_end, init_mem_end - init_mem_start,
            init_code_start, init_code_end, init_code_end - init_code_start,
            init_data_start, init_data_end, init_data_end - init_data_start,
            kstack_start, kstack_end, kstack_end - kstack_start,
            kernel_code_start, kernel_code_end, kernel_code_end - kernel_code_start,
            kernel_data_start, kernel_data_end, kernel_data_end - kernel_data_start,
            kernel_bss_start, kernel_bss_end, kernel_bss_end - kernel_bss_start,
            kernel_percpu_start, kernel_percpu_end, kernel_percpu_end - kernel_percpu_start,
            kernel_end, early_placement_address, (char *)early_placement_address - kernel_end);
}
*/

void show_mem_at(uintptr_t addr, size_t nbytes, size_t bytes_per_line)
{
    unsigned char *mem = (unsigned char *)addr;
    int col = term_fg_color(0xf080f0);
    for (size_t i = 0; i < nbytes; ) {
        printk("\t0x%08x%08x - ", (uintptr_t)(mem + i) >> 32, mem + i);
        for (size_t j = 0; j < bytes_per_line; ++j)
            printk("%02x ", mem[i++]);
        printk("\n");
    }
    term_fg_color(col);
}
