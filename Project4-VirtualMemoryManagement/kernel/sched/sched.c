#include <os/list.h>
#include <os/mm.h>
#include <pgtable.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/irq.h>
#include <screen.h>
#include <stdio.h>
#include <assert.h>
#define RR
//#define PRIORITY
pcb_t pcb[NUM_MAX_TASK];
const ptr_t pid0_stack_0 = INIT_KERNEL_STACK + 2 * PAGE_SIZE;
const ptr_t pid0_stack_1 = INIT_KERNEL_STACK + 4 * PAGE_SIZE;
pcb_t pid0_pcb_core0 = {
    .pid = 0,
    .kernel_sp = (ptr_t)(INIT_KERNEL_STACK + 0xe70),
    .user_sp = (ptr_t)pid0_stack_0,
    .pgdir = 0x5e000000,
    .preempt_count = 0
};
pcb_t pid0_pcb_core1 = {
    .pid = 0,
    .kernel_sp = (ptr_t)(INIT_KERNEL_STACK + 0x2e70),
    .user_sp = (ptr_t)pid0_stack_1,
    .pgdir = 0x5e000000,
    .preempt_count = 0
};
LIST_HEAD(ready_queue);
LIST_HEAD(wait_queue);
pcb_t *dequeue(list_head* queue);
void do_unblock_time(pcb_t *item);
/* current running task PCB */
pcb_t ** volatile current_running;
//extern uint32_t *sch_timer;
/* global process id */
pid_t process_id = 1;

uint16_t score(priority_t p, uint64_t time){
    return p * 5 + ((get_ticks() - time) / TIMER_INTERVAL); 
}
pcb_t *findnext(list_head *queue){
    pcb_t *find = NULL;
    list_node_t *p = queue->next;
    pcb_t *pcb_f = list_entry(p, pcb_t, list);
    find = pcb_f;
    uint8_t s = 0;
     while (p != queue){ 
        pcb_f = list_entry(p, pcb_t, list);
         if (score(pcb_f->priority, pcb_f->add_tick) > s){
            find = pcb_f;
            s = score(pcb_f->priority, pcb_f->add_tick);
        }
        p = p->next;
    } 
    return find;
}

pcb_t *findnext_core()
{
    list_node_t *p = ready_queue.next;
    uint8_t mask = 0x3;
    uint8_t core;
    int id = get_current_cpu_id();
    pcb_t *find;
    while(p != &ready_queue)
    {
        find = list_entry(p, pcb_t, list);
        core = find->mask & mask;
        if ((id == 0) && (core == 1 || core == 3)){
            list_del(p);
            return find;
        }
        if ((id == 1) && (core == 2 || core == 3)){
            list_del(p);
            return find;
        }
        p = p->next;
    }
    return NULL;
}

void free_all_page(pid_t pid)
{
      list_head *head = &(pcb[pid - 1].plist);
      while(!list_empty(head))
      {
          page_t *cur = list_entry(head->next, page_t, list);
          freePage(cur->pa);
          list_del(head->next);
      }
}

void do_scheduler(void)
{
      //*sch_timer = get_ticks();
      current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
        #ifdef PRIORITY
        pcb_t *next_running = findnext(&ready_queue);
        list_del(&next_running->list);
        #endif
        #ifdef RR
        pcb_t *next_running = findnext_core();//dequeue(&ready_queue);
        if (next_running == NULL)
        {
            if ( (*current_running)->status == TASK_RUNNING && 
                (get_current_cpu_id() == 0 && (*current_running)->mask == 0x1 
              || get_current_cpu_id() == 1 && (*current_running)->mask == 0x2
              || (*current_running)->mask == 0x3))
                next_running = *current_running;
            else
                next_running = (get_current_cpu_id() == 0) ? &pid0_pcb_core0 : &pid0_pcb_core1; 
        }
        #endif
        pcb_t *temp = ((pcb_t *)(*current_running));
        if(((pcb_t *)(*current_running))->status == TASK_RUNNING &&
            ((pcb_t *)(*current_running))->pid != 0 && 
            *current_running != next_running){ 
            enqueue(&ready_queue, ((pcb_t *)(*current_running)));
            ((pcb_t *)(*current_running))->status = TASK_READY;
        }
        next_running->status = TASK_RUNNING;
        ((pcb_t *)(*current_running))->add_tick = get_ticks();
        *current_running = next_running;
        process_id = ((pcb_t *)(*current_running))->pid;
        // change pgdir
        set_satp(SATP_MODE_SV39, BOOT_ASID, (uint64_t)(next_running->pgdir) >> 12);
        local_flush_tlb_all();
        switch_to(temp, next_running);
} 

int do_kill(pid_t pid)
{
    // Already exited
    if (pid == 1){
        prints("\n> Permission denied.\n");
        return 0;
    }
    if (pcb[pid - 1].pid == 0)
        return 0;
    // Modified status
    pcb[pid - 1].status = TASK_EXITED;
    pcb[pid - 1].pid = 0;
    // Delete it from all block or ready queues
    list_del(&pcb[pid - 1].list);
    free_all_page(pid);   
    // Zombie check
    if (pcb[pid - 1].mode == ENTER_ZOMBIE_ON_EXIT)
        zombie_check(pcb[pid - 1].parent);
    // Release all locks it handled
 //   for (int i = 0; i < pcb[pid - 1].lock_num; i++)
 //        if (pcb[pid - 1].lock[i] > 0) 
 //           kernel_binsemop(pcb[pid - 1].lock[i],BINSEM_OP_UNLOCK);
    return 1;
}

void do_process_show()
{
    prints("> Process table\n");
     for (int i = 0; i < NUM_MAX_TASK; i++){
        if(pcb[i].pid){
            switch(pcb[i].status){
                case TASK_RUNNING:
                    if (*current_running == &pcb[i] && get_current_cpu_id() == 0)
                        prints("PID %d: %s MASK = %d\n", pcb[i].pid, "RUNNING (core 0)", pcb[i].mask);
                    if (*current_running == &pcb[i] && get_current_cpu_id() == 1)
                        prints("PID %d: %s MASK = %d\n", pcb[i].pid, "RUNNING (core 1)", pcb[i].mask);
                    if (*current_running != &pcb[i] && get_current_cpu_id() == 0)
                        prints("PID %d: %s MASK = %d\n", pcb[i].pid, "RUNNING (core 1)", pcb[i].mask);
                    if (*current_running != &pcb[i] && get_current_cpu_id() == 1)
                        prints("PID %d: %s MASK = %d\n", pcb[i].pid, "RUNNING (core 0)", pcb[i].mask);
                    break;
                case TASK_BLOCKED:
                    prints("PID %d: %s MASK = %d\n", pcb[i].pid, "BLOCKED", pcb[i].mask);
                    break;
                case TASK_EXITED:
                    prints("PID %d: %s MASK = %d\n", pcb[i].pid, "EXITED", pcb[i].mask);
                    break;
                case TASK_READY:
                    prints("PID %d: %s MASK = %d\n", pcb[i].pid, "READY", pcb[i].mask);
                    break;
            }
        }
    }
}

pid_t do_getpid()
{
    current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
    return ((pcb_t *)(*current_running))->pid;    
}

void do_sleep(uint32_t sleep_time)
{
    // TODO: sleep(seconds)
    // note: you can assume: 1 second = `timebase` ticks
    // 1. block the ((pcb_t *)(*current_running))
    current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
    ((pcb_t *)(*current_running))->status = TASK_BLOCKED;
    ((pcb_t *)(*current_running))->sleep_add_core = get_current_cpu_id();
    // 2. create a timer which calls `do_unblock` when timeout
    timer_create(&do_unblock, &(((pcb_t *)(*current_running))->list), sleep_time * time_base);
    // 3. reschedule because the ((pcb_t *)(*current_running)) is blocked.
    do_scheduler();
}

void do_block(pcb_t *pcb_node, list_head *queue)
{
    // Block the pcb task into the block queue
    current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
    enqueue(queue, pcb_node);
    ((pcb_t *)(*current_running))->status = TASK_BLOCKED;
    do_scheduler();
}

void enqueue(list_head* queue, pcb_t* item)
{
    list_add_tail(&item->list, queue);
}

pcb_t *dequeue(list_head* queue)
{
    if (!list_empty(queue)){
        pcb_t* tmp = list_entry(queue->next, pcb_t, list);
        list_del(queue->next);
        return tmp;
    }
}

void do_unblock(list_node_t *item)
{
    // Unblock the `pcb` from the block queue
        pcb_t* unblock_task = list_entry(item, pcb_t, list);
        list_del(item);
        unblock_task->status = TASK_READY;
        enqueue(&ready_queue, unblock_task);
    //do_scheduler();
}

void zombie_check(pid_t pid)
{
    if (pid == 0)
        return ;
    else
    {
        do_unblock(&pcb[pid - 1].list); 
    }
}

void do_exit()
{
    current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
    free_all_page((*current_running)->pid);
    if (((pcb_t *)(*current_running))->mode == AUTO_CLEANUP_ON_EXIT)
    {
        ((pcb_t *)(*current_running))->pid = 0;
        ((pcb_t *)(*current_running))->status = TASK_EXITED;
    }else
    {
        ((pcb_t *)(*current_running))->status = TASK_EXITED;
        ((pcb_t *)(*current_running))->pid = 0;
        zombie_check(((pcb_t *)(*current_running))->parent);
    }
    do_scheduler();
}

int do_waitpid(pid_t pid)
{
    current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
    if (pcb[pid - 1].pid != 0){ 
        if (pcb[pid - 1].status != TASK_EXITED){
            pcb[pid - 1].parent = ((pcb_t *)(*current_running))->pid;
            do_block(((pcb_t *)(*current_running)), &wait_queue);
        }
    }
    return pid;
}
