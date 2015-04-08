#ifndef __YAK_KERNEL_H__
#define __YAK_KERNEL_H__

#include <yak/lib/types.h>

#include <yak/video/printk.h>

#define assert(condition) \
    ((condition) ? (void)0 : \
     panic("Assertion failed: %s, function %s, file %s, line %u.", \
        #condition, __FUNCTION__, __FILE__, __LINE__))

static inline void mem_barrier(void)
{
    asm volatile("" ::: "memory");
}

static inline void cpu_relax(void)
{
    asm volatile("pause" ::: "memory");
}

static inline void local_irq_enable(void)
{
    asm volatile("sti");
}

static inline void local_irq_disable(void)
{
    asm volatile("cli");
}

#endif
