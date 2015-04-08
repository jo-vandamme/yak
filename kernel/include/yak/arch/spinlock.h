#ifndef __YAK_SPINLOCK_H__
#define __YAK_SPINLOCK_H__

#include <yak/kernel.h>
#include <yak/lib/atomic.h>

#define LOCK_BUSY 1

typedef struct spinlock spinlock_t;
struct spinlock
{
    uint32_t lock;
    flags_t flags;
};

void spin_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);
int spin_trylock(spinlock_t *lock);

#endif
