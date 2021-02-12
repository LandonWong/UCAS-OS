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
#include <pgtable.h>
#include <stdio.h>
#include <os/time.h>
#include <os/syscall.h>
#include <os/futex.h>
#include <../test/test.h>
#include <../include/user_programs.h>
#include <../include/os/elf.h>
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
uint8_t  cancel_map_flag = 0;

static void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
    pcb_t *pcb, int argc, char *argv[])
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
        pt_regs->regs[2] = USER_STACK_ADDR - 0xc0;
        pt_regs->regs[3] = (reg_t)__global_pointer$;
        pt_regs->regs[4] = (reg_t)pcb;
        pt_regs->sepc = entry_point;
        pt_regs->sstatus = (1 << 18);
        pt_regs->scause = 0;
        pt_regs->sbadaddr = 0;
    } 
    pt_regs->regs[10] = argc;
    pt_regs->regs[11] = argv;
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
uintptr_t alloc_page_helper_user(uintptr_t kva, uintptr_t pgdir)
{
    return alloc_page_helper(kva, pgdir, 1);
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
    pcb[0].pgdir = allocPage();
    // copy kernel page
    kmemcpy((char *)pcb[0].pgdir, (char *)pa2kva(0x5e000000), PAGE_SIZE);
    ptr_t kernel_sp = (uint64_t)alloc_page_helper(KERNEL_STACK_ADDR - PAGE_SIZE, pcb[0].pgdir, 0) + PAGE_SIZE - 128;
    ptr_t user_sp = (uint64_t)alloc_page_helper(USER_STACK_ADDR - PAGE_SIZE, pcb[0].pgdir, 1) + PAGE_SIZE - 128;
    pcb[0].mask = 0x3;
    // Load ELF
    ptr_t start_pos = (ptr_t)load_elf(_elf___test_test_shell_elf,
                                     _length___test_test_shell_elf,
                                     pcb[0].pgdir,
                                     alloc_page_helper_user);
    // Init pcb stack
    init_pcb_stack(kernel_sp, user_sp, start_pos, &pcb[0], NULL, NULL);
    // Give new pgdir sp
    pcb[0].user_sp = USER_STACK_ADDR - 128;
    pcb[0].pgdir = kva2pa(pcb[0].pgdir);
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
    init_pcb_stack(pcb[i].kernel_sp, pcb[i].user_sp, task->entry_point, &pcb[i], arg, 0);
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

int match_elf(char *file_name)
{
    for (int i = 1; i < 5; i++)
        if (kstrcmp(file_name, elf_files[i].file_name) == 0)
            return i;
    return -1;
}

void do_show_exec()
{
    for (int i = 1; i < 5; i++)
        prints("%s ", elf_files[i].file_name);
    prints("\n");
}
pid_t do_exec(const char *file_name, int argc, char* argv[], spawn_mode_t mode)
{
    // Match ELF file
    int prog_id = match_elf(file_name);
    if (prog_id == -1)
    {
        prints("> Error: No such program `%s\'.\n", file_name);
        return -1;
    }
    current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
    // Find one PCB
    int i = findpid();
    if (i <= 0){ 
        prints("> Tasks full.\n");
        while(1);
    }
    // Set status
    pcb[i].status = TASK_READY;
    pcb[i].type = USER_PROCESS;
    pcb[i].pid = i + 1;
    pcb[i].mode = mode;
    pcb[i].mask = ((pcb_t *)*current_running) -> mask;
    pcb[i].pnum = 2;
    // Setup pgdir
    pcb[i].pgdir = allocPage();
    // copy kernel page
    kmemcpy((char *)pcb[i].pgdir, (char *)pa2kva(0x5e000000), PAGE_SIZE);
    // Alloc dual stack
    ptr_t kernel_sp = (uint64_t)alloc_page_helper(KERNEL_STACK_ADDR - PAGE_SIZE, pcb[i].pgdir, 0) + PAGE_SIZE;
    ptr_t user_sp = (uint64_t)alloc_page_helper(USER_STACK_ADDR - PAGE_SIZE, pcb[i].pgdir, 1) + PAGE_SIZE - 0xc0;
    init_list_head(&(pcb[i].plist));
    page_t *kstack = (page_t *)kmalloc(sizeof(page_t));
    page_t *ustack = (page_t *)kmalloc(sizeof(page_t));
    kstack->pa = kva2pa((uint64_t)kernel_sp - PAGE_SIZE);
    kstack->va = KERNEL_STACK_ADDR - PAGE_SIZE;
    kstack->atsd = 1;
    ustack->pa = kva2pa((uint64_t)user_sp + 0xc0 - PAGE_SIZE);
    ustack->va = USER_STACK_ADDR - PAGE_SIZE;
    ustack->atsd = 1;
    list_add_tail(&(kstack->list), &(pcb[i].plist));
    list_add_tail(&(ustack->list), &(pcb[i].plist));
    // Load ELF file
    ptr_t start_pos = (ptr_t)load_elf(elf_files[prog_id].file_content,
                                      *elf_files[prog_id].file_length,
                                      pcb[i].pgdir,
                                      alloc_page_helper_user);
    // Copy new argv
    uintptr_t new_argv = USER_STACK_ADDR - 0xc0;
    uint64_t *kargv = user_sp;
    for (int j = 0; j < argc; j++){
        *(kargv + j) = (uint64_t)new_argv + 0x10 * (j + 1);
        kmemcpy((uint64_t)user_sp + 0x10 * (j + 1), argv[j], 0x10 * (j + 1));
    }
    // Init pcb stack
    init_pcb_stack(kernel_sp, user_sp, start_pos, &pcb[i], argc, new_argv);
    // Give new pgdir sp
    pcb[i].user_sp = USER_STACK_ADDR - 0xc0;
    pcb[i].pgdir = kva2pa(pcb[i].pgdir);
    // Debug info
    /*
    prints("> Successfully exec `%s\',\n  start at 0x%lx,\n  kernel_sp at 0x%lx,\n  pgdir at 0x%lx,\n  argc = %d,\n  argv at 0x%lx,\n  argv[0] = %s (at 0x%lx),\n  argv[1] = %s (at 0x%lx),\n  argv[2] = %s (at 0x%lx).\n", 
            elf_files[prog_id].file_name, 
            start_pos, 
            pcb[i].kernel_sp,
            pcb[i].pgdir,
            argc,
            argv,
            argv[0], argv[0],
            argv[1], argv[1],
            argv[2], argv[2]);
    */
    // Add to ready queue
    enqueue(&ready_queue, &pcb[i]);
    return i + 1;
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
    syscall[SYSCALL_EXEC        ] = &do_exec;
    syscall[SYSCALL_SHOW_EXEC   ] = &do_show_exec;
    syscall[SYSCALL_SHMPGET     ] = &shm_page_get;
    syscall[SYSCALL_SHMPDT      ] = &shm_page_dt;
}

void cancel_id_mapping()
{
    uint64_t va = 0x50200000;
    uint64_t pgdir = 0xffffffc05e000000;
    uint64_t vpn_2 = (va >> 30) & ~(~0 << 9);
    uint64_t vpn_1 = (va >> 21) & ~(~0 << 9);
    PTE *level_2 = (PTE *)pgdir + vpn_2;
    PTE *map_pte = (PTE *)pa2kva(get_pa(*level_2)) + vpn_1;
    *level_2 = 0;
    *map_pte = 0;
}

int main()
{ 
    init_syscall();
    if (get_current_cpu_id() == 0){
        spin_lock_init(&kernel_lock);
        spin_lock_acquire(&kernel_lock);
        current_running = &current_running_core0;
        *current_running = &pid0_pcb_core0;
        local_flush_tlb_all();
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
    if (!cancel_map_flag)
        cancel_id_mapping();
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
