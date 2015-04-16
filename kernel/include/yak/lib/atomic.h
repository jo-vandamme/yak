#ifndef __YAK_ATOMIC_H__
#define __YAK_ATOMIC_H__

#define atomic_xadd(P, V)       __sync_fetch_and_add((P), (V))
#define cmpxchg(P, O, N)        __sync_val_compare_and_swap((P), (O), (N))
#define atomic_inc(P)           __sync_add_and_fetch((P), 1);
#define atomic_dec(P)           __sync_add_and_fetch((P), -1);
#define atomic_add(P, V)        __sync_add_and_fetch((P), (V))
#define atomic_set_bit(P, V)    __sync_or_and_fetch((P), 1 << (V))
#define atomic_clear_bit(P, V)  __sync_and_and_fetch((P), ~(1 << (V)))

static inline void *xchg_64(void *ptr, void *x)
{
    asm volatile("xchgq %0, %1"
                 : "=r"((uint64_t)x)
                 : "m" (*(volatile uint64_t *)ptr), "0"((uint64_t)x)
                 : "memory");
    return x;
}

static inline uint32_t xchg_32(void *ptr, uint32_t x)
{
    asm volatile("xchgl %0, %1"
                 : "=r"((uint32_t)x)
                 : "m"(*(volatile uint32_t *)ptr), "0"(x)
                 : "memory");
    return x;
}

static inline uint16_t xchg_16(void *ptr, uint16_t x)
{
    asm volatile("xchgw %0, %1"
                 : "=r"((uint16_t)x)
                 : "m"(*(volatile uint16_t *)ptr), "0"(x)
                 : "memory");
    return x;
}

static inline uint8_t atomic_bit_set_and_test(void *ptr, int x)
{
    uint8_t out;
    asm volatile("lock; bts %2, %1\n"
                 "sbb %0, %0"
                 : "=r"(out), "=m"(*(volatile uint64_t *)ptr)
                 : "Ir"(x)
                 : "memory");
    return out;
}

#endif