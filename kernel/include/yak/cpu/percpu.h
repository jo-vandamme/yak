#ifndef __YAK_PERCPU_H__
#define __YAK_PERCPU_H__

#include <yak/lib/types.h>
#include <yak/cpu/cpu.h>

extern const char kernel_percpu_start[];

void percpu_init(unsigned int id, uintptr_t percpu_base);

#define DEFINE_PERCPU(type, var)                        \
    type __attribute__((section(".data.percpu"))) __percpu_##var

#define DECLARE_PERCPU(type, var)                       \
    extern type __attribute__((section(".data.percpu"))) __percpu_##var

#define this_cpu_id                                     \
    ({                                                  \
        uintptr_t id;                                   \
        asm volatile("movq %%gs:8, %0" : "=r"(id));     \
        id;                                             \
    })

#define this_cpu_data                                   \
    ({                                                  \
        uintptr_t self;                                 \
        asm volatile("movq %%gs:0, %0" : "=r"(self));   \
        self;                                           \
    })

#define percpu_offset(var)                              \
    (((uintptr_t)&__percpu_##var) - ((uintptr_t)kernel_percpu_start))

#define __percpu_get(percpu, var)                       \
    (*({                                                \
       (typeof(__percpu_##var) *)((uintptr_t)percpu + percpu_offset(var)); \
    }))

#define percpu_get(var) __percpu_get(this_cpu_data, var)

#define MSR_FS_BASE 0xc0000100
#define MSR_GS_BASE 0xc0000101

static inline void set_fs(uint64_t val)
{
    write_msr(MSR_FS_BASE, val);
}

static inline void set_gs(uint64_t val)
{
    write_msr(MSR_GS_BASE, val);
}

static inline uint64_t get_gs(void)
{
    return read_msr(MSR_GS_BASE);
}

#endif
