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
#include <asm/sbidef.h>
#include <csr.h>
#define TIMEIN
//#define NONP

extern void __global_pointer$();
extern void ret_from_exception();
pcb_t * volatile current_running_core0;
pcb_t * volatile current_running_core1;

extern spin_lock_t kernel_lock;

uint64_t kernel_stack[NUM_MAX_TASK];
uint64_t user_stack[NUM_MAX_TASK];

static void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
    pcb_t *pcb, void *arg)
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
    pt_regs->regs[10] = arg;
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

static void init_shell()
{
    current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
    init_list_head(&ready_queue);
    init_list_head(&sleep_queue);
    pcb[0].pid = 1;
    pcb[0].status = TASK_READY;
    pcb[0].type = USER_PROCESS;
    void *arg = 0;
    pcb[0].kernel_sp = kernel_stack[0];
    pcb[0].user_sp = user_stack[0];
    pcb[0].mask = 0x3;
    init_pcb_stack(pcb[0].kernel_sp, pcb[0].user_sp, &test_shell, &pcb[0], arg);
    enqueue(&ready_queue, &pcb[0]);
}

int findpid()
{
    int i;
    for (i = 1; i < NUM_MAX_TASK; i++){
        if (pcb[i].pid == 0)
            return i;
    }
    return -1;
}

pid_t init_pcb(task_info_t *task, void *arg, spawn_mode_t mode, uint8_t mask)
{
    current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
    int i = findpid();
    if (i <= 0){ 
        printk("> Tasks full.\n");
        while(1);
    }
    pcb[i].status = TASK_READY;
    pcb[i].pid = i + 1;
    pcb[i].mode = mode;
    pcb[i].type = task->type;
    pcb[i].kernel_sp = kernel_stack[i];
    pcb[i].user_sp = user_stack[i];
    if (mask == 0)
        mask = ((pcb_t *)*current_running) -> mask;
    pcb[i].mask = mask;
    init_pcb_stack(pcb[i].kernel_sp, pcb[i].user_sp, task->entry_point, &pcb[i], arg);
    enqueue(&ready_queue, &pcb[i]);
    return i + 1;
}

void init_all_stack()
{
    int i;
    for (i = 0; i < NUM_MAX_TASK; i++){
        kernel_stack[i] = allocPage(1);
        user_stack[i] = allocPage(1);
    }
}

void error_syscall(void){
    printk("> INFO: Undefined syscall! \n");
    printk("> Abort\n");
    while(1);
}
void kernel_taskset(uint8_t mask, pid_t pid)
{
    if (pcb[pid - 1].pid == 0){
        prints("> Error: Process %d not found.\n", pid);
        return;
    }
    pcb[pid - 1].mask = mask;    
}

static void init_syscall(void)
{
    // initialize system call table.
    int i;
    for(i = 0; i < NUM_SYSCALLS; i++)
        syscall[i] = &error_syscall;
    syscall[SYSCALL_SLEEP       ] = &do_sleep;
    syscall[SYSCALL_SEMGET      ] = &kernel_semget;
    syscall[SYSCALL_BINSEMOP    ] = &kernel_binsemop;
    syscall[SYSCALL_WRITE       ] = &screen_write;
    syscall[SYSCALL_CURSOR      ] = &screen_move_cursor;
    syscall[SYSCALL_REFLUSH     ] = &screen_reflush;
    syscall[SYSCALL_YIELD       ] = &do_scheduler;
    syscall[SYSCALL_GET_TIMEBASE] = &get_time_base;
    syscall[SYSCALL_GET_TICK    ] = &get_ticks;
    syscall[SYSCALL_SERIAL_READ ] = &serial_getchar;
    syscall[SYSCALL_SERIAL_WRITE] = &screen_write_ch;
    syscall[SYSCALL_SCREEN_CLEAR] = &screen_clear;
    syscall[SYSCALL_SPAWN       ] = &init_pcb;
    syscall[SYSCALL_KILL        ] = &do_kill;
    syscall[SYSCALL_PS          ] = &do_process_show;
    syscall[SYSCALL_GETPID      ] = &do_getpid;
    syscall[SYSCALL_EXIT        ] = &do_exit;
    syscall[SYSCALL_WAITPID     ] = &do_waitpid;
    syscall[SYSCALL_DOBLOCK     ] = &do_block;
    syscall[SYSCALL_DOUNBLOCK   ] = &do_unblock;
    syscall[SYSCALL_SEMWAIT     ] = &kernel_sem_wait;
    syscall[SYSCALL_SEMSIGNAL   ] = &kernel_sem_signal;
    syscall[SYSCALL_SEMBROADCAST] = &kernel_sem_broadcast;
    syscall[SYSCALL_BARRIERWAIT ] = &kernel_barrier_wait;
    syscall[SYSCALL_MBOX_OPEN   ] = &kernel_mbox_open;
    syscall[SYSCALL_MBOX_CLOSE  ] = &kernel_mbox_close;
    syscall[SYSCALL_MBOX_SEND   ] = &kernel_mbox_send;
    syscall[SYSCALL_MBOX_RECV   ] = &kernel_mbox_recv;
    syscall[SYSCALL_SCREENSCROLL] = &kernel_screen_scroll_check;
    syscall[SYSCALL_TASKSET     ] = &kernel_taskset;
}

int main()
{ 
    init_syscall();
    if (get_current_cpu_id() == 0){
        spin_lock_init(&kernel_lock);
        spin_lock_acquire(&kernel_lock);
        current_running = &current_running_core0;
        *current_running = &pid0_pcb_core0;
        init_all_stack();
        init_shell();
        printk("> Core 0 enter main.\n\r");
        sbi_send_ipi(NULL);
    }else{
        current_running = &current_running_core1;
        *current_running = &pid0_pcb_core1;
        printk("> Core 1 enter main.\n\r");
    }

    time_base = sbi_read_fdt(TIMEBASE);

    init_exception();

    if (get_current_cpu_id() == 0){
        init_screen();
    }
#ifdef TIMEIN
    sbi_set_timer(get_ticks() + time_base / 100);
    spin_lock_release(&kernel_lock);
#endif
    while (1) { 
#ifdef TIMEIN
    enable_interrupt();
        __asm__ __volatile__("wfi\n\r");
#endif
#ifdef NONP
        do_scheduler();
#endif
    };
    return 0;
}
