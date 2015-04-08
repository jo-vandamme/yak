#ifndef __YAK_GDT_H__
#define __YAK_GDT_H__

#ifndef __ASSEMBLY__

#include <yak/lib/types.h>

typedef struct
{
    u16_t limit;
    u64_t base;
} __attribute__((packed)) gdt_ptr_t;

void gdt_init(void);

//struct cpu;
//void gdt_init(struct cpu *cpu);

#endif

#define SEG_NULL  0x00
#define SEG_KCODE 0x01
#define SEG_KDATA 0x02
#define SEG_KPCPU 0x03
#define SEG_UCODE 0x04
#define SEG_UDATA 0x05
#define SEG_TSS   0x06

#endif
