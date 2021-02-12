#include <os/lock.h>
#include <os/sched.h>
#include <atomic.h>
#define MAX_BINSEM 256
binsem_t *binsem_list[MAX_BINSEM];
void spin_lock_init(spin_lock_t *lock)
{
    lock->status = UNLOCKED;
}

int spin_lock_try_acquire(spin_lock_t *lock)
{
    return atomic_swap_d(1, &lock->status);
}

volatile void spin_lock_acquire(spin_lock_t *lock)
{
    while(spin_lock_try_acquire(lock) == LOCKED);
}

void spin_lock_release(spin_lock_t *lock)
{
    lock->status = UNLOCKED;
}

void do_mutex_lock_init(mutex_lock_t *lock)
{
    init_list_head(&lock->block_queue);
    lock->lock.status = UNLOCKED;
}

void do_mutex_lock_acquire(mutex_lock_t *lock)
{
    if(lock->lock.status == LOCKED)
        do_block(current_running, &lock->block_queue);
    else
        lock->lock.status = LOCKED;
}

void do_mutex_lock_release(mutex_lock_t *lock)
{
    if(list_empty(&lock->block_queue))
        lock->lock.status = UNLOCKED;
    else{
        do_unblock(&lock->block_queue);
    } 
}
uint8_t id_gen()
{
    uint64_t random = get_ticks();
    uint64_t mask = 0xff;
    return (uint8_t)(random & mask);
}

int kernel_binsemget(int key)
{
    uint8_t id;
    for(int i = 0; i < MAX_BINSEM; i++){
        if (binsem_list[i])
            if (binsem_list[i]->key == key){
                return binsem_list[i]->id;
            }
    }
    // Generate id
    for(id = id_gen(); binsem_list[id] != NULL; id = id_gen());
    // Alloc memory space
    binsem_t *new = (binsem_t *) kmalloc(sizeof(binsem_t));
    // Init
    new->id = id;
    new->key = key;
    new->status = BIN_UNLOCKED;
    init_list_head(&new->block_queue);
    // Add to list
    binsem_list[id] = new;
    return id;
}

int kernel_binsemop(int binsem_id, int op)
{
    binsem_t *bin = binsem_list[binsem_id];
    if (op == BINSEM_OP_LOCK){ 
        if(bin->status == BIN_LOCKED)
            do_block(current_running, &bin->block_queue);
        else
            bin->status = BIN_LOCKED;
    } else if (op == BINSEM_OP_UNLOCK){   
        if(list_empty(&bin->block_queue))
            bin->status = BIN_UNLOCKED;
        else{
            do_unblock(bin->block_queue.next);
         } 
    } else {
        printk("> ERROR: Invalid binsem opcode.\n");
        while(1);
    } 
}
