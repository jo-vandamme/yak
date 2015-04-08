#include <yak/kernel.h>
#include <yak/arch/spinlock.h>

void spin_lock(spinlock_t *lock)
{
    for (;;) {
        if (!xchg_32(lock, EBUSY))
            return;
        while (*lock)
            cpu_relax();
    }
}

void spin_unlock(spinlock_t *lock)
{
    mem_barrier();
    *lock = 0;
}

int spin_trylock(spinlock_t *lock)
{
    return xchg_32(lock, EBUSY);
}

