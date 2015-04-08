#ifndef __YAK_SPINLOCK_H__
#define __YAK_SPINLOCK_H__

#include <yak/kernel.h>
#include <yak/atomic.h>

// code from http://locklessinc.com/articles/locks/

#define EBUSY 1
typedef uint32_t spinlock_t;

void spin_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);
int spin_trylock(spinlock_t *lock);

#endif
