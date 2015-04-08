#include <yak/kernel.h>
#include <yak/lib/atomic.h>
#include <yak/arch/spinlock.h>

// code from http://locklessinc.com/articles/locks/

void spin_lock(spinlock_t *lock)
{
    for (;;) {
        lock->flags = local_irq_save();
        if (!xchg_32(&lock->lock, LOCK_BUSY))
            return;
        while (lock->lock) {
            local_irq_restore(lock->flags);
            cpu_relax();
        }
    }
}

void spin_unlock(spinlock_t *lock)
{
    mem_barrier();
    lock->lock = 0;
    local_irq_restore(lock->flags);
}

int spin_trylock(spinlock_t *lock)
{
    lock->flags = local_irq_save();
    uint32_t res = xchg_32(&lock->lock, LOCK_BUSY);
    if (res)
        local_irq_restore(lock->flags);

    return res;
}

