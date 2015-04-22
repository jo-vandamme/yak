#ifndef __MEM_H__
#define __MEM_H__

#include <stddef.h>
#include <yak/lib/types.h>

void mem_init(void);
void mem_percpu_init(int ncpus, uintptr_t *area, uintptr_t *stack);
void mem_reclaim_init(void);
void mem_reloc_boot_structs(void);

void show_mem_at(uintptr_t addr, size_t nbytes, size_t bytes_per_line);

extern const char image_start[];
extern const char kernel_end[];

/*
void print_image_sections(void);

extern char image_end[];
extern char boot_mem_start[];
extern char boot_mem_end[];
extern char init_mem_start[];
extern char init_mem_end[];
extern char kstack_start[];
extern char kstack_end[];
extern char kernel_code_start[];
extern char kernel_code_end[];
extern char kernel_data_start[];
extern char kernel_data_end[];
extern char kernel_bss_start[];
extern char kernel_bss_end[];
extern char kernel_percpu_start[];
extern char kernel_percpu_end[];
extern char init_code_start[];
extern char init_code_end[];
extern char init_data_start[];
extern char init_data_end[];
*/

#endif
