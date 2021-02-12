/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *         The kernel's entry, where most of the initialization work is done.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include <common.h>
#include <os/irq.h>
#include <os/mm.h>
#include <os/sched.h>
#include <screen.h>
#include <sbi.h>
#include <stdio.h>
#include <os/time.h>
#include <os/syscall.h>
#include <os/futex.h>
#include <test.h>

#include <csr.h>
#define TIMEIN
//#define NONP
//#define SLEEP
//#define LOCK_TEST
#define PRIOR_TEST
//#define DEADL_TEST
uint32_t *sch_timer;
extern void __global_pointer$();
extern void ret_from_exception();
static void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
    pcb_t *pcb)
{
    regs_context_t *pt_regs =
        (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    size_t i;
    // Init all regs as zero
    for(i = 0;i < 32;i++){
        pt_regs->regs[i] = 0;    
    }   
    // Init Supervisor-Level regs
    if (pcb->type == USER_PROCESS || pcb->type == USER_THREAD){    
        pt_regs->regs[2] = user_stack;
        pt_regs->regs[3] = (reg_t)__global_pointer$;
        pt_regs->regs[4] = (reg_t)pcb;
        pt_regs->sepc = entry_point;
        pt_regs->sstatus = 0;
        pt_regs->scause = 0;
        pt_regs->sbadaddr = 0;
    } 
    // Set sp
    pcb->kernel_sp = kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t);
    switchto_context_t *st_regs = (switchto_context_t *)pcb->kernel_sp;
    pcb->user_sp = user_stack;
    // Init regs value saved in switch to
    if (pcb->type == USER_PROCESS || pcb->type == USER_THREAD)
        st_regs->regs[0] = &ret_from_exception;
    else
        st_regs->regs[0] = entry_point;
    st_regs->regs[1] = user_stack;
     for(i = 2;i < 14;i++){
        st_regs->regs[i] = 0;    
    }   
    
}

static void init_pcb()
{
    int i;
    // Init the PCB ready queue
    sch_timer = (uint32_t *)kmalloc(sizeof(sch_timer));
    init_list_head(&ready_queue);
    init_list_head(&sleep_queue);
    // Init Process 0 memory stack
    // Add tasks
    task_info_t *task = NULL;
    // Tasks
    #ifdef SLEEP
      for(i = 0;i  < num_timer_tasks;i++){ 
        task = timer_tasks[i];
    #endif
    #ifdef DEADL_TEST
      for(i = 0;i < num_sched2_tasks;i++){ 
        task = sched2_tasks[i];
    #endif
    #ifdef PRIOR_TEST
      for(i = 0;i < num_priority_tasks;i++){ 
        task = priority_tasks[i];
    #endif
    #ifdef LOCK_TEST
      for(i = 0;i < num_sched2_tasks + num_lock_tasks;i++){ 
        task = (i < num_sched2_tasks) ? sched2_tasks[i] : lock2_tasks[i - num_sched2_tasks];
    #endif
        // For multi-stack kernel
        pcb[i].kernel_sp = allocPage(1);
        pcb[i].user_sp = allocPage(1);
        pcb[i].pid = i + 1;
        pcb[i].status = TASK_READY;
        pcb[i].type = task->type;
        pcb[i].priority = task->priority;
        // Init pcb stack
        init_pcb_stack(pcb[i].kernel_sp, pcb[i].user_sp, 
                       task->entry_point, &pcb[i]);
        // Add this pcb to ready_queue
        pcb[i].add_tick = get_ticks();
        enqueue(&ready_queue, &pcb[i]);
    }  
    current_running = &pid0_pcb;
}

void error_syscall(void){
    printk("> INFO: Undefined syscall! \n");
    printk("> Abort\n");
    while(1);
}

static void init_syscall(void)
{
    // initialize system call table.
    int i;
    for(i = 0; i < NUM_SYSCALLS; i++)
        syscall[i] = &error_syscall;
    syscall[SYSCALL_SLEEP       ] = &do_sleep;
    syscall[SYSCALL_BINSEMGET   ] = &kernel_binsemget;
    syscall[SYSCALL_BINSEMOP    ] = &kernel_binsemop;
    syscall[SYSCALL_WRITE       ] = &screen_write;
    syscall[SYSCALL_CURSOR      ] = &screen_move_cursor;
    syscall[SYSCALL_REFLUSH     ] = &screen_reflush;
    syscall[SYSCALL_SCHED       ] = &do_scheduler;
    syscall[SYSCALL_GET_TIMEBASE] = &get_time_base;
    syscall[SYSCALL_GET_TICK    ] = &get_ticks;
}

// jump from bootloader.
// The beginning of everything >_< ~~~~~~~~~~~~~~
int main()
{ 
    // init Process Control Block (-_-!)
    init_pcb();
    printk("> [INIT] PCB initialization succeeded.\n\r");

    // read CPU frequency
    time_base = sbi_read_fdt(TIMEBASE);
	
    // init futex mechanism
    //init_system_futex();
 
    // init interrupt (^_^)
    init_exception();
    printk("> [INIT] Interrupt processing initialization succeeded.\n\r");

    // init system call table (0_0)
    init_syscall();
    printk("> [INIT] System call initialized successfully.\n\r");

    // fdt_print(riscv_dtb);

    //init screen (QAQ)
    init_screen();
    printk("> [INIT] SCREEN initialization succeeded.\n\r");

    // TODO:
    // Setup timer interrupt and enable all interrupt
#ifdef TIMEIN
    sbi_set_timer(get_ticks() + TIMER_INTERVAL);
#endif
    while (1) {
        // (QAQQQQQQQQQQQ)
        // If you do non-preemptive scheduling, you need to use it
        // to surrender control do_scheduler();
#ifdef TIMEIN
        enable_interrupt();
        __asm__ __volatile__("wfi\n\r":::);
#endif
#ifdef NONP
        do_scheduler();
#endif
     };
    return 0;
}
