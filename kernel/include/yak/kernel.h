#ifndef __YAK_KERNEL_H__
#define __YAK_KERNEL_H__

#include <yak/lib/types.h>

#include <yak/video/printk.h>

#define assert(condition) \
    ((condition) ? (void)0 : \
     panic("Assertion failed: %s, function %s, file %s, line %u.", \
        #condition, __FUNCTION__, __FILE__, __LINE__))

#define __packed __attribute__((packed))
#define __unused __attribute__((unused))

typedef uint32_t flags_t;

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

static inline flags_t local_irq_save(void)
{
    flags_t flags;
    asm volatile("pushf; pop %0; cli" : "=rm" (flags));
    return flags;
}

static inline void local_irq_restore(flags_t flags)
{
    asm volatile("push %0; popf" : : "rm" (flags));
}

#endif
