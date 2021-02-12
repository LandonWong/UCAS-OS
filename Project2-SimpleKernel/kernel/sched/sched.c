#include <os/list.h>
#include <os/mm.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/irq.h>
#include <screen.h>
#include <stdio.h>
#include <assert.h>
//#define RR
#define PRIORITY
pcb_t pcb[NUM_MAX_TASK];
const ptr_t pid0_stack = INIT_KERNEL_STACK + PAGE_SIZE;
pcb_t pid0_pcb = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack,
    .user_sp = (ptr_t)pid0_stack,
    .preempt_count = 0
};

LIST_HEAD(ready_queue);
pcb_t *dequeue(list_head* queue);
void do_unblock_time(pcb_t *item);
/* current running task PCB */
pcb_t * volatile current_running;
extern uint32_t *sch_timer;
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

void do_scheduler(void)
{
      *sch_timer = get_ticks();
      if (!list_empty(&ready_queue)){ 
        #ifdef PRIORITY
        pcb_t *next_running = findnext(&ready_queue);
        list_del(&next_running->list);
        #endif
        #ifdef RR
        pcb_t *next_running = dequeue(&ready_queue);
        #endif
        pcb_t *temp = current_running;
        if(current_running->status == TASK_RUNNING && current_running->pid != 0){ 
            enqueue(&ready_queue, current_running);
            current_running->status = TASK_READY;
        }
        next_running->status = TASK_RUNNING;
        current_running->add_tick = get_ticks();
        current_running = next_running;
        process_id = current_running->pid;
        vt100_move_cursor(current_running->cursor_x,
                          current_running->cursor_y);
        screen_cursor_x = current_running->cursor_x;
        screen_cursor_y = current_running->cursor_y;
        switch_to(temp, next_running);
     }
} 

void do_sleep(uint32_t sleep_time)
{
    // TODO: sleep(seconds)
    // note: you can assume: 1 second = `timebase` ticks
    // 1. block the current_running
    current_running->status = TASK_BLOCKED;
    // 2. create a timer which calls `do_unblock` when timeout
    timer_create(&do_unblock, &current_running->list, sleep_time * time_base);
    // 3. reschedule because the current_running is blocked.
    do_scheduler();
}

void do_block(list_node_t *pcb_node, list_head *queue)
{
    // Block the pcb task into the block queue
    enqueue(queue, pcb_node);
    current_running->status = TASK_BLOCKED;
    do_scheduler();
}

void enqueue(list_head* queue, pcb_t* item)
{
    list_add_tail(&item->list, queue);
}

pcb_t *dequeue(list_head* queue)
{
    pcb_t* tmp = list_entry(queue->next, pcb_t, list);
    list_del(queue->next);
    return tmp;
}

void do_unblock(list_node_t *item)
{
    // Unblock the `pcb` from the block queue
    pcb_t* unblock_task = list_entry(item, pcb_t, list);
    list_del(item);
    unblock_task->status = TASK_READY;
    enqueue(&ready_queue, unblock_task);
   // do_scheduler();
}
