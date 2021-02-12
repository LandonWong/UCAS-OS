#include <stdatomic.h>
#include <stdint.h>
#include <mthread.h>
#include <assert.h>
#include <sys/syscall.h>
int key_global = 4097;
int mthread_spin_init(mthread_spinlock_t *lock)
{
    // TODO:
}
int mthread_spin_destroy(mthread_spinlock_t *lock) {
    // TODO:
}
int mthread_spin_trylock(mthread_spinlock_t *lock)
{
    // TODO:
}
int mthread_spin_lock(mthread_spinlock_t *lock)
{
    // TODO:
}
int mthread_spin_unlock(mthread_spinlock_t *lock)
{
    // TODO:
}

int mthread_mutex_init(mthread_mutex_t *lock)
{
    // TODO:
    lock->key = key_global;
    key_global += rand() % 5 + 1;
    lock->id  = binsemget(lock->key);
}
int mthread_mutex_destroy(mthread_mutex_t *lock) 
{
    // TODO:
    lock->key = -1;
    lock->id  = -1;
}
int mthread_mutex_trylock(mthread_mutex_t *lock) 
{
    // TODO:
}
int mthread_mutex_lock(mthread_mutex_t *lock) 
{
    // TODO:
    binsemop(lock->id, BINSEM_OP_LOCK);
}
int mthread_mutex_unlock(mthread_mutex_t *lock)
{
    // TODO:
    binsemop(lock->id, BINSEM_OP_UNLOCK);
}

int mthread_barrier_init(mthread_barrier_t * barrier, unsigned count)
{
    // TODO:
    barrier->key = key_global;
    key_global += rand() % 5 + 1;
    barrier->id  = binsemget(barrier->key);
    barrier->count = count;
}
int mthread_barrier_wait(mthread_barrier_t *barrier)
{
    barrier_wait(barrier->id, barrier->count);
}
int mthread_barrier_destroy(mthread_barrier_t *barrier)
{
    // TODO:
    barrier->key = 0;
    barrier->id = 0;
    barrier->count = 0;
}

int mthread_cond_init(mthread_cond_t *cond)
{
    // TODO:
    cond->key = key_global;
    key_global += rand() % 5 + 1;
    cond->id  = binsemget(cond->key);
}
int mthread_cond_destroy(mthread_cond_t *cond) 
{  
    // TODO:
    cond->key = -1;
    cond->id  = -1;
}
int mthread_cond_wait(mthread_cond_t *cond, mthread_mutex_t *mutex)
{ 
    // TODO:
    sem_wait(cond->id, mutex->id);
} 

int mthread_cond_signal(mthread_cond_t *cond)
{ 
    // TODO:
    sem_signal(cond->id);
} 

int mthread_cond_broadcast(mthread_cond_t *cond)
{ 
    // TODO:
    sem_broadcast(cond->id);
} 

int mthread_semaphore_init(mthread_semaphore_t *sem, int val)
{
    // TODO:
}
int mthread_semaphore_up(mthread_semaphore_t *sem)
{
    // TODO:
}
int mthread_semaphore_down(mthread_semaphore_t *sem)
{
    // TODO:
}
int mthread_semaphore_destroy(mthread_semaphore_t *sem)
{
    // TODO:
}
