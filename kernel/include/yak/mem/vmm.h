#ifndef __YAK_VMM_H__
#define __YAK_VMM_H__

#include <yak/lib/types.h>
#include <yak/config.h>
 
#define VMM_RECURSIVE_SLOT  (510ull)

// get the index within each paging structure
#define VMM_PML4_IDX(virt)  ((((uintptr_t)(virt)) >> 39) & 0x1ffull)
#define VMM_PML3_IDX(virt)  ((((uintptr_t)(virt)) >> 30) & 0x1ffull)
#define VMM_PML2_IDX(virt)  ((((uintptr_t)(virt)) >> 21) & 0x1ffull)
#define VMM_PML1_IDX(virt)  ((((uintptr_t)(virt)) >> 12) & 0x1ffull)

// get the base address of the paging structures
#define VMM_PML1_BASE       ((0xffffull << 48) | (VMM_RECURSIVE_SLOT << 39))
#define VMM_PML2_BASE       (VMM_PML1_BASE     | (VMM_RECURSIVE_SLOT << 30))
#define VMM_PML3_BASE       (VMM_PML2_BASE     | (VMM_RECURSIVE_SLOT << 21))
#define VMM_PML4_BASE       (VMM_PML3_BASE     | (VMM_RECURSIVE_SLOT << 12))

// get the paging structures for the given address
#define VMM_PML4(virt)      ((uint64_t *)VMM_PML4_BASE)
#define VMM_PML3(virt)      ((uint64_t *)(VMM_PML3_BASE + (((virt) >> 27) & 0x00001ff000ull)))
#define VMM_PML2(virt)      ((uint64_t *)(VMM_PML2_BASE + (((virt) >> 18) & 0x003ffff000ull)))
#define VMM_PML1(virt)      ((uint64_t *)(VMM_PML1_BASE + (((virt) >>  9) & 0x7ffffff000ull)))

// kernel conversions
#define VMM_P2V(phys) ((uintptr_t)phys + VIRTUAL_BASE)
#define VMM_V2P(virt) ((uintptr_t)virt - VIRTUAL_BASE)

uintptr_t map(uintptr_t virt, uintptr_t phys, int flags);
void unmap(uintptr_t virt, unsigned int npages);
uintptr_t map_temp(uintptr_t phys);

//uintptr_t virt_to_phys(uintptr_t virt);
//uintptr_t phys_to_virt(uintptr_t phys);

void flush_tlb(uintptr_t virt);

void vmm_init(void);

#endif
