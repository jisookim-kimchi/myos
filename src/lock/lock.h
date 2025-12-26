#ifndef LOCK_H
#define LOCK_H

typedef int spinlock_t;

void spin_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);

#endif