#ifndef __YAK_CPU_H__
#define __YAK_CPU_H__

#include <yak/lib/types.h>

static inline void write_msr(uint32_t msr, uint64_t val)
{
    uint32_t high = val >> 32;
    uint32_t low = val & 0xffffffff;
    asm volatile("wrmsr" : : "a"(low), "d"(high), "c"(msr));
}

static inline uint64_t read_msr(uint32_t msr)
{
    uint32_t low, high;
    asm volatile("rdmsr" : "=&a"(low), "=&d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

#endif
