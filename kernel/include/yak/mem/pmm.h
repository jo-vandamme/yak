#ifndef __YAK_PMM_H__
#define __YAK_PMM_H__

#include <yak/lib/types.h>

uintptr_t alloc_frame(void);
void free_frame(uintptr_t frame);

void pmm_init(uintptr_t kernel_virt_start, uintptr_t kernel_virt_end);

void print_mem_stat_local(void);
void print_mem_stat_global(void);

#endif
