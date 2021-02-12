
#include <common.h>
#include <os/irq.h>
#include <os/ioremap.h>
#include <os/mm.h>
#include <os/fs.h>
#include <os/sched.h>
#include <screen.h>
#include <sbi.h>
#include <pgtable.h>
#include <stdio.h>
#include <os/time.h>
#include <os/fs.h>
#include <os/syscall.h>
#include <os/futex.h>
#include <../test/test.h>
#include <../include/user_programs.h>
#include <../include/os/elf.h>
#include <asm/sbidef.h>
#include <csr.h>
#include <asm.h>
#include <assert.h>

#include <plic.h>
#include <emacps/xemacps_example.h>
#include <net.h>
#define BOARD
//#define QEMU
#define NET
#define TIMEIN
//#define NONP

extern void __global_pointer$();
extern void ret_from_exception();
pcb_t * volatile current_running_core0;
pcb_t * volatile current_running_core1;

extern spin_lock_t kernel_lock;

uint64_t kernel_stack[NUM_MAX_TASK];
uint64_t user_stack[NUM_MAX_TASK];

int cancel_map_flag = 0;

static void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
    pcb_t *pcb, int argc, char *argv[])
{
    current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
    regs_context_t *pt_regs =
        (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    size_t i;
    // Init all regs as zero
    for(i = 0;i < 32;i++){
        pt_regs->regs[i] = 0;    
    }   
    // Init Supervisor-Level regs
    if (pcb->type == USER_PROCESS || pcb->type == USER_THREAD){    
        pt_regs->regs[2] = USER_STACK_ADDR + ((pcb_t *)*current_running) -> child_num * PAGE_SIZE - 0xc0;
        regs_context_t *old = (*current_running)->kernel_sp + SWITCH_TO_SIZE;
        pt_regs->regs[3] = old->regs[3];
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
    //init_pcb_stack(pcb[i].kernel_sp, pcb[i].user_sp, task->entry_point, &pcb[i], arg);
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

void set_priority(pid_t pid, priority_t priority)
{
    pcb[pid - 1].priority = priority;
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
    for (int i = 1; i < 9; i++)
        if (kstrcmp(file_name, elf_files[i].file_name) == 0)
            return i;
    return -1;
}

void do_show_exec()
{ 
    for (int i = 1; i < 9; i++)
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
    pcb[i].priority = 4;
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
    pcb[i].entry_point = start_pos;
    // Copy new argv
    uintptr_t new_argv = USER_STACK_ADDR - 0xc0;
    uint64_t *kargv = user_sp;
    for (int j = 0; j < argc; j++){
        *(kargv + j) = (uint64_t)new_argv + 0x10 * (j + argc);
        kmemcpy((uint64_t)user_sp + 0x10 * (j + argc), argv[j], 0x10);
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

pid_t do_run(char *name, int argc, char* argv[], int length)
{  
    prints("> Run %s.(%d)\n", name,length);
    screen_reflush();
    uintptr_t entry = load_file(name, length);
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
    pcb[i].mode = 0;
    pcb[i].mask = ((pcb_t *)*current_running) -> mask;
    pcb[i].priority = 4;
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
    ptr_t start_pos = (ptr_t)load_elf(entry,
                                      length,
                                      pcb[i].pgdir,
                                      alloc_page_helper_user);
    pcb[i].entry_point = start_pos;
    // Copy new argv
    uintptr_t new_argv = USER_STACK_ADDR - 0xc0;
    uint64_t *kargv = user_sp;
    for (int j = 0; j < argc; j++){
        *(kargv + j) = (uint64_t)new_argv + 0x10 * (j + argc);
        kmemcpy((uint64_t)user_sp + 0x10 * (j + argc), argv[j], 0x10);
    }
    // Init pcb stack
    init_pcb_stack(kernel_sp, user_sp, start_pos, &pcb[i], argc, new_argv);
    // Give new pgdir sp
    pcb[i].user_sp = USER_STACK_ADDR - 0xc0;
    pcb[i].pgdir = kva2pa(pcb[i].pgdir);
    // Add to ready queue
    enqueue(&ready_queue, &pcb[i]);
    return i + 1;
}
pid_t do_exec_thread(uintptr_t entry_point, int argc, char* argv[], spawn_mode_t mode)
{
    // Match ELF file
    current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
    // Find one PCB
    int i = findpid();
    if (i <= 0){ 
        prints("> Tasks full.\n");
        while(1);
    }
    // Set status
    pcb[i].status = TASK_READY;
    pcb[i].type = USER_THREAD;
    pcb[i].pid = i + 1;
    pcb[i].mode = mode;
    pcb[i].mask = ((pcb_t *)*current_running) -> mask;
    ((pcb_t *)*current_running) -> child_num ++;
    pcb[i].priority = 4;
    pcb[i].pnum = 2;
    // Setup pgdir
    pcb[i].pgdir = ((pcb_t *)*current_running) -> pgdir;
    // Alloc dual stack
    ptr_t kernel_sp = (uint64_t)alloc_page_helper(KERNEL_STACK_ADDR + PAGE_SIZE * ((pcb_t *)*current_running)->child_num - PAGE_SIZE, pa2kva(pcb[i].pgdir), 0) + PAGE_SIZE;
    ptr_t user_sp = (uint64_t)alloc_page_helper(USER_STACK_ADDR + ((pcb_t *)*current_running) -> child_num * PAGE_SIZE - PAGE_SIZE, pa2kva(pcb[i].pgdir), 1) + PAGE_SIZE - 0xc0;
    // Load ELF file
    ptr_t start_pos = entry_point;
    // Copy new argv
    uintptr_t new_argv = USER_STACK_ADDR + ((pcb_t *)*current_running) -> child_num * PAGE_SIZE - 0xc0;
    uint64_t *kargv = user_sp;
    for (int j = 0; j < argc; j++){
        *(kargv + j) = (uint64_t)new_argv + 0x10 * (j + argc);
        kmemcpy((uint64_t)user_sp + 0x10 * (j + argc), argv[j], 0x10);
    }
    // Init pcb stack
    init_pcb_stack(kernel_sp, user_sp, start_pos, &pcb[i], argc, new_argv);
    // Give new pgdir sp
    pcb[i].user_sp = USER_STACK_ADDR + ((pcb_t *)*current_running) -> child_num * PAGE_SIZE - 0xc0;
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
    syscall[SYSCALL_SPAWN       ] = &do_exec_thread;
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
    syscall[SYSCALL_RUN         ] = &do_run;
    syscall[SYSCALL_SHOW_EXEC   ] = &do_show_exec;
    syscall[SYSCALL_SHMPGET     ] = &shm_page_get;
    syscall[SYSCALL_SHMPDT      ] = &shm_page_dt;
    syscall[SYSCALL_NET_RECV    ] = &do_net_recv;
    syscall[SYSCALL_NET_SEND    ] = &do_net_send;
    syscall[SYSCALL_NET_IRQ_MODE] = &do_net_irq_mode;
    syscall[SYSCALL_MKFS        ] = &mkfs;
    syscall[SYSCALL_STATFS      ] = &statfs;
    syscall[SYSCALL_READDIR     ] = &read_dir;
    syscall[SYSCALL_MKDIR       ] = &mkdir;
    syscall[SYSCALL_RMDIR       ] = &rmdir;
    syscall[SYSCALL_CGDIR       ] = &change_dir;
    syscall[SYSCALL_TOUCH       ] = &touch;
    syscall[SYSCALL_CAT         ] = &cat;
    syscall[SYSCALL_OPEN        ] = &open;
    syscall[SYSCALL_CLOSE       ] = &close;
    syscall[SYSCALL_FREAD       ] = &read;
    syscall[SYSCALL_FWRITE      ] = &write;
    syscall[SYSCALL_PRIORITY    ] = &set_priority;
    syscall[SYSCALL_LINKS       ] = &links;
    syscall[SYSCALL_LINKH       ] = &links;
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
    cancel_map_flag = 1;
}

void setup_ethernet()
{
    uint32_t slcr_bade_addr = 0, ethernet_addr = 0;

    // get_prop_u32(_dtb, "/soc/slcr/reg", &slcr_bade_addr);
    slcr_bade_addr = sbi_read_fdt(SLCR_BADE_ADDR);
    printk("[slcr] phy: 0x%x\n\r", slcr_bade_addr);

    // get_prop_u32(_dtb, "/soc/ethernet/reg", &ethernet_addr);
    ethernet_addr = sbi_read_fdt(ETHERNET_ADDR);
    printk("[ethernet] phy: 0x%x\n\r", ethernet_addr);

    uint32_t plic_addr = 0;
    // get_prop_u32(_dtb, "/soc/interrupt-controller/reg", &plic_addr);
    plic_addr = sbi_read_fdt(PLIC_ADDR);
    printk("[plic] plic: 0x%x\n\r", plic_addr);

    uint32_t nr_irqs = sbi_read_fdt(NR_IRQS);
    // get_prop_u32(_dtb, "/soc/interrupt-controller/riscv,ndev", &nr_irqs);
    printk("[plic] nr_irqs: 0x%x\n\r", nr_irqs);

    XPS_SYS_CTRL_BASEADDR =
        (uintptr_t)ioremap((uint64_t)slcr_bade_addr, NORMAL_PAGE_SIZE);
    xemacps_config.BaseAddress =
#ifdef QEMU
        (uintptr_t)ioremap((uint64_t)ethernet_addr, 9 * NORMAL_PAGE_SIZE);
        xemacps_config.BaseAddress = (uint64_t)xemacps_config.BaseAddress + 0x8000;
#endif
#ifdef BOARD
        (uintptr_t)ioremap((uint64_t)ethernet_addr, NORMAL_PAGE_SIZE);
#endif
    uintptr_t _plic_addr =
        (uintptr_t)ioremap((uint64_t)plic_addr, 0x4000 * NORMAL_PAGE_SIZE);
    // XPS_SYS_CTRL_BASEADDR = slcr_bade_addr;
    // xemacps_config.BaseAddress = ethernet_addr;
    xemacps_config.DeviceId        = 0;
    xemacps_config.IsCacheCoherent = 0;

    printk(
        "[slcr_bade_addr] phy:%x virt:%lx\n\r", slcr_bade_addr,
        XPS_SYS_CTRL_BASEADDR);
    printk(
        "[ethernet_addr] phy:%x virt:%lx\n\r", ethernet_addr,
        xemacps_config.BaseAddress);
    printk("[plic_addr] phy:%x virt:%lx\n\r", plic_addr, _plic_addr);
    plic_init(_plic_addr, nr_irqs);
    
    long status = EmacPsInit(&EmacPsInstance);
    if (status != XST_SUCCESS) {
        printk("Error: initialize ethernet driver failed!\n\r");
        assert(0);
    }
}

int main()
{ 
    init_syscall();
    time_base = sbi_read_fdt(TIMEBASE);
    if (get_current_cpu_id() == 0){
        spin_lock_init(&kernel_lock);
        spin_lock_acquire(&kernel_lock);
        current_running = &current_running_core0;
        *current_running = &pid0_pcb_core0;
#ifdef NET
	    setup_ethernet();
#endif
        local_flush_tlb_all();
        init_shell();
    if (!check_fs())
        mkfs();
        printk("> Core 0 enter main.\n\r");
        sbi_send_ipi(NULL);
    }else{
        current_running = &current_running_core1;
        *current_running = &pid0_pcb_core1;
        printk("> Core 1 enter main.\n\r");
    }


    init_exception();
    if (!cancel_map_flag)
        cancel_id_mapping();
    if (get_current_cpu_id() == 0){
        init_screen();
    }
#ifdef TIMEIN
    sbi_set_timer(get_ticks() + time_base / 200);
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
