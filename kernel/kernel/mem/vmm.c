#include <yak/config.h>
#include <yak/initcall.h>
#include <yak/lib/string.h>
#include <yak/lib/utils.h>
#include <yak/video/printk.h>
#include <yak/mem/pmm.h>
#include <yak/mem/vmm.h>

char temp_mapping[PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));

inline void flush_tlb(uintptr_t virt)
{
    asm volatile("invlpg (%0)" : : "b"(virt) : "memory");
}
/*
uintptr_t virt_to_phys(uintptr_t virt)
{
    uintptr_t *pt = ((uintptr_t *)PT_BASE) + (1024 * PD_IDX(virt));
    return pt[PT_IDX(virt)] & 0xfffff000;
}

uintptr_t phys_to_virt(uintptr_t phys)
{
    return phys + VIRTUAL_BASE;
}
*/

uintptr_t map_temp(uintptr_t phys)
{
    uintptr_t virt = map((uintptr_t)temp_mapping, phys, 3);
    return virt + phys % PAGE_SIZE;
}

static inline uintptr_t clear_frame(uintptr_t frame)
{
    map_temp((uintptr_t)frame);
    memset((void *)temp_mapping, 0, PAGE_SIZE);
    return frame;
}

uintptr_t map(uintptr_t virt, uintptr_t phys, int flags)
{
    uint64_t *table, idx;
    phys = align_down(phys, PAGE_SIZE);
    virt = align_down(virt, PAGE_SIZE);

    table = VMM_PML4(virt);
    idx = VMM_PML4_IDX(virt);
    if (!(table[idx] & 0x1)) {
        //printk("\33\x0f\x0f alloc PML4 table %08x%08x[%u]\n", (uintptr_t)table >> 32, table, idx);
        table[idx] = clear_frame(alloc_frame()) | 3;
        flush_tlb(virt);
    }

    table = VMM_PML3(virt);
    idx = VMM_PML3_IDX(virt);
    if (!(table[idx] & 0x1)) {
        //printk("\33\x0f\x0f alloc PML3 table %08x%08x[%u]\n", (uintptr_t)table >> 32, table, idx);
        table[idx] = clear_frame(alloc_frame()) | 3;
        flush_tlb(virt);
    }

    table = VMM_PML2(virt);
    idx = VMM_PML2_IDX(virt);
    if (!(table[idx] & 0x1)) {
        //printk("\33\x0f\x0f alloc PML2 table %08x%08x[%u]\n", (uintptr_t)table >> 32, table, idx);
        table[idx] = clear_frame(alloc_frame()) | 3;
        flush_tlb(virt);
    }

    table = VMM_PML1(virt);
    idx = VMM_PML1_IDX(virt);
    /*if (table[idx] & 0x1) {
        panic("<vmm> A page is already mapped to 0x%08x%08x\n", virt >> 32, virt);
    }*/
    table[idx] = phys | flags;
    flush_tlb(virt);

    return virt;
}

static uint64_t *get_table(uintptr_t virt, int level, uint64_t *index)
{
    uint64_t *table = 0;
    switch (level) {
        case 1:
            table = VMM_PML1(virt); 
            *index = VMM_PML1_IDX(virt);
            break;
        case 2:
            table = VMM_PML2(virt); 
            *index = VMM_PML2_IDX(virt);
            break;
        case 3:
            table = VMM_PML3(virt); 
            *index = VMM_PML3_IDX(virt);
            break;
        case 4: 
            table = VMM_PML4(virt); 
            *index = VMM_PML4_IDX(virt);
            break;
        default:
            panic("<vmm> No table for level %u\n", level);
    }
    return table;
}

// TODO make it work with 2MB pages
static void unmap_tables(uintptr_t virt, int level)
{
    if (level > 3)
        return;

    uint64_t idx = 0;
    uint64_t *table = get_table(virt, level, &idx);
    for (unsigned int i = 0; i < 512; ++i)
        if (table[i] & 0x1)
            return;

    table = get_table(virt, level + 1, &idx);
    free_frame(table[idx] & 0xfffffffffffff000);
    table[idx] = 0;
    unmap_tables(virt, level + 1);
}

void unmap(uintptr_t virt, unsigned int npages)
{
    uint64_t *table, idx;
    virt = align_down(virt, PAGE_SIZE);

    // unmap the pages
    for (unsigned int i = 0; i < npages; ++i, virt += PAGE_SIZE) {
        table = VMM_PML1(virt);
        idx = VMM_PML1_IDX(virt);
        table[idx] = 0;

        // unmap the tables if necessary
        if (i == npages-1 || table != VMM_PML1(virt + PAGE_SIZE)) {
            unmap_tables(virt, 1);
        }
        flush_tlb(virt);
    }
}

INIT_CODE void vmm_init(void)
{
    free_frame(VMM_V2P(temp_mapping));
}
