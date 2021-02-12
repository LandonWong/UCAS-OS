#include <os/lock.h>
#include <os/sched.h>
#include <atomic.h>
#include <mailbox.h>
#define MAX_BINSEM 256
#define MAX_NUM 256
sem_t *sem_list[MAX_BINSEM];
mbox_t *mbox_list[MAX_NUM];
int global_key = 0;
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
    current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
    if(lock->lock.status == LOCKED)
        do_block(((pcb_t *)(*current_running)), &lock->block_queue);
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
int id_gen()
{
    uint64_t random = get_ticks();
    return (char)(random);
}


int kernel_semget(int key)
{
    int id;
    for(int i = 0; i < MAX_BINSEM; i++){
        if (sem_list[i])
            if (sem_list[i]->key == key){
                return sem_list[i]->id;
            }
    }
    // Generate id
    for(id = id_gen(); sem_list[id] != NULL; id = id_gen());
    // Alloc memory space
    sem_t *new = (sem_t *) kmalloc(sizeof(sem_t));
    // Init
    new->id = id;
    new->key = key;
    new->status = BIN_UNLOCKED;
    init_list_head(&new->block_queue);
    // Add to list
    sem_list[id] = new;
    return id;
}

int kernel_binsemop(int binsem_id, int op)
{
    current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
    binsem_t *bin = sem_list[binsem_id];
    if (op == BINSEM_OP_LOCK){ 
        if(bin->status == BIN_LOCKED)
            do_block(*current_running, &bin->block_queue);
        else{
           current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
           bin->status = BIN_LOCKED;
           ((pcb_t *)(*current_running))->lock[((pcb_t *)(*current_running))->lock_num ++] = binsem_id;
        }
    } else if (op == BINSEM_OP_UNLOCK){   
        if(list_empty(&bin->block_queue)){
            current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
            for(int i = 0; i < ((pcb_t *)(*current_running))->lock_num; i++)
                if (((pcb_t *)(*current_running))->lock[i] == binsem_id)
                {
                    ((pcb_t *)(*current_running))->lock[i] = -1;
                    break;
                }
            bin->status = BIN_UNLOCKED;
        }
        else{
            current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
            do_unblock(bin->block_queue.next);
        } 
    } else {
        prints("> ERROR: Invalid binsem opcode.\n");
        while(1);
    } 
}

int kernel_sem_wait(int sem_id, int lock_id)
{
    current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
    sem_t *cond = sem_list[sem_id];
    cond->status ++;
    kernel_binsemop(lock_id, BINSEM_OP_UNLOCK);
    current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
    do_block(*current_running, &cond->block_queue);
    kernel_binsemop(lock_id, BINSEM_OP_LOCK);
}

int kernel_sem_signal(int sem_id)
{
    sem_t *cond = sem_list[sem_id];
    if (cond->status > 0)
        do_unblock(cond->block_queue.next);
    cond->status --;
}

int kernel_sem_broadcast(int sem_id)
{
    sem_t *cond = sem_list[sem_id];
    list_node_t *p = cond->block_queue.next, *q;
    while(p != &cond->block_queue)
    {
        q = p->next;
        do_unblock(p);
        p = q;
    } 
    cond->status = 0;
}

int kernel_barrier_wait(int id, int count)
{
    current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
    sem_t *barr = sem_list[id];
    barr->status ++;
    if (count == barr->status)
    {
        barr->status = 0;
        list_node_t *p = barr->block_queue.next, *q;
        while(p != &barr->block_queue)
        {
            q = p->next;
            do_unblock(p);
            p = q;
        }
    }
    else
    {
        do_block(*current_running, &barr->block_queue);
    }
}

int kernel_keygen()
{
    global_key = (global_key + 1 + rand() % 5) % 4096;
    return global_key;    
}

mailbox_t kernel_mbox_open(char *name)
{
    int i = 0;
    for (i = 0;i < MAX_NUM; i++)
    {
        if((mbox_list[i] != NULL) && (!strcmp(name, mbox_list[i]->name))){
            mbox_list[i]->cited++;
            return mbox_list[i]->id;
        }
    }
    int id;
    while(1){
        id = id_gen();
        if (mbox_list[id] == NULL)
            break;
    }
    mbox_t *new = (mbox_t *)kmalloc(sizeof(mbox_t));
    new->id = id;
    mbox_list[new->id] = new;
    strcpy(mbox_list[new->id]->name, name);
    mbox_list[new->id]->cited++;
    mbox_list[new->id]->used_size = mbox_list[new->id]->head = mbox_list[new->id]->tail = 0;
    mbox_list[new->id]->empty_id = kernel_semget(kernel_keygen());
    mbox_list[new->id]->full_id = kernel_semget(kernel_keygen());
    mbox_list[new->id]->lock_id = kernel_semget(kernel_keygen());
    return new->id;
}

void kernel_mbox_close(mailbox_t mailbox)
{
    mbox_list[mailbox] = NULL;
}

void kernel_mbox_send(mailbox_t mailbox, void *msg, int msg_length)
{
    mbox_t *box = mbox_list[mailbox];
    kernel_binsemop(box->lock_id, BINSEM_OP_LOCK);
    while(MSG_MAX_SIZE - box->used_size < msg_length)
        kernel_sem_wait(box->empty_id, box->lock_id);
    if(MSG_MAX_SIZE - box->tail < msg_length)
    {
        memcpy((char *)(box->buffer + box->tail), (char *)msg, MSG_MAX_SIZE - box->tail);
        box->tail = msg_length - (MSG_MAX_SIZE - box->tail);
        memcpy((char *)box->buffer, (char *)(msg + msg_length - box->tail), box->tail);
    }
    else
    {
        memcpy((char *)(box->buffer + box->tail), (char *)msg, msg_length);
        box->tail += msg_length;
    }
    box->used_size += msg_length; 
    kernel_sem_broadcast(box->full_id);
    kernel_binsemop(box->lock_id, BINSEM_OP_UNLOCK);
}

void kernel_mbox_recv(mailbox_t mailbox, void *msg, int msg_length)
{
    mbox_t *box = mbox_list[mailbox];
    kernel_binsemop(box->lock_id, BINSEM_OP_LOCK);
    while(box->used_size < msg_length)
        kernel_sem_wait(box->full_id, box->lock_id);
    if(MSG_MAX_SIZE - box->head < msg_length)
    {
        memcpy((char *)msg, (char *)(box->buffer + box->head), MSG_MAX_SIZE - box->head);
        box->head = msg_length - (MSG_MAX_SIZE - box->head);
        memcpy((char *)(msg + msg_length - box->head), (char *)box->buffer, box->head);
    }
    else
    {
        memcpy((char *)msg, (char *)(box->buffer + box->head), msg_length);
        box->head += msg_length;
    }
    box->used_size -= msg_length;
    kernel_sem_broadcast(box->empty_id);
    kernel_binsemop(box->lock_id, BINSEM_OP_UNLOCK);
}
