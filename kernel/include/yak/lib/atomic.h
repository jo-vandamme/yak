#ifndef __YAK_ATOMIC_H__
#define __YAK_ATOMIC_H__

typedef struct
{
    uint64_t low;
    uint64_t high;
} __attribute__((aligned(16), packed)) uint128_t;

// if (*src == *cmp) *src = new,  return true;
// else              *cmp = *src, return false;
// CMPXCHG16B m128 - compare rdx:rax with m128. If equal, set ZF and load rcx:rbx into m128.
// Else, clear ZF and load m128 into rdx:rax.
static inline uint8_t cas16(uint128_t *src, uint128_t *cmp, uint128_t new)
{
    uint8_t result;
    asm volatile(
            "lock; cmpxchg16b %1\n"
            "setz %0"
            : "=q"(result), "+m"(*src), "+a"(cmp->low), "+d"(cmp->high)
            : "b"(new.low), "c"(new.high) : "cc");
    if (!result)
        asm volatile("pause" ::: "memory");
    return result;
}

// is there anything faster than this to load a 
// 16 bytes value atomically ?
static inline uint128_t atomic_load16(uint128_t *src)
{
    uint128_t res = { .low = 0, .high = 0 };
    asm volatile(
            "lock; cmpxchg16b %0"
            : "+m"(*src), "+a"(res.low), "+d"(res.high)
            : "b"(src->low), "c"(src->high) : "cc");
    return res;
}

#define atomic_add(src, inc) \
    asm volatile("lock; addq %1, %0" : "+m"(*(src)) : "ir"((inc)));

#define atomic_sub(src, dec) \
    asm volatile("lock; subq %1, %0" : "+m"(*(src)) : "ir"((dec)));


#define atomic_xadd(P, V)       __sync_fetch_and_add((P), (V))
#define cmpxchg(P, O, N)        __sync_val_compare_and_swap((P), (O), (N))
#define atomic_inc(P)           __sync_add_and_fetch((P), 1);
#define atomic_dec(P)           __sync_add_and_fetch((P), -1);
//#define atomic_add(P, V)        __sync_add_and_fetch((P), (V))
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
