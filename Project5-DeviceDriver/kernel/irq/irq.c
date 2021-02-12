#include <os/irq.h>
#include <emacps/xemacps_example.h>
#include <os/lock.h>
#include <os/time.h>
#include <os/sched.h>
#include <os/string.h>
#include <stdio.h>
#include <os/mm.h>
#include <assert.h>
#include <pgtable.h>
#include <sbi.h>
#include <screen.h>
#include <csr.h>
#include <plic.h>
#define MAX_PNUM 10000
extern list_head net_recv_queue;
extern list_head net_send_queue;
handler_t irq_table[IRQC_COUNT];
handler_t exc_table[EXCC_COUNT];
uintptr_t riscv_dtb;
spin_lock_t kernel_lock;
int bid = 10000;

extern uint64_t read_sip();
void handle_irq(regs_context_t *regs, int irq)
{
    // Clear IXR and judge type
    int type = JudgeIntr(&EmacPsInstance);
    if (ReadRXSR(&EmacPsInstance))
        ClearRXSR(&EmacPsInstance);
    if (ReadTXSR(&EmacPsInstance))
        ClearTXSR(&EmacPsInstance);
    if (type % 2 == 0 && !list_empty(&net_send_queue))
        do_unblock(net_send_queue.next);
    if (type && !list_empty(&net_recv_queue))
        do_unblock(net_recv_queue.next);
    // let PLIC know that handle_irq has been finished
    plic_irq_eoi(irq);
}

void disk2mem(int block, int num, uintptr_t pa, uintptr_t va)
{
    sbi_sd_read(pa, num, block);
    prints("[swap] from SD block %d to MEM 0x%lx (v0x%lx) (#blocks %d)\n",
            block, pa, va, num);
    screen_reflush();
}

void mem2disk(int block, int num, uintptr_t pa, uintptr_t va)
{
    sbi_sd_write(pa, num, block);
    prints("[swap] from MEM 0x%lx (v0x%lx) to SD block %d (#blocks %d)\n",
            pa, va, block, num);
    screen_reflush();
}

int clock_unblock(pcb_t *pcb, uintptr_t va)
{
    list_node_t *p = pcb->plist.next;
    page_t *info = NULL;
    while (p != &pcb->plist)
    {
        info = list_entry(p, page_t, list);
        if (va == info->va)
        {
            pcb->pnum++;
            disk2mem(info->block, 8, info->pa, info->va);
            uintptr_t *pte = info->pte;
            set_attribute(pte, (1 << 6) | (1 << 7) | 0x1); 
            info->atsd = 0;
            info->block = 0;
            return 1;
        }
        p = p->next;
    }
    return 0;
}

void clock_block(pcb_t *pcb)
{
    list_node_t *p = pcb->plist.next;
    page_t *info = NULL;
    while (p != &pcb->plist)
    {
        info = list_entry(p, page_t, list);
        if(!info->atsd)
        {
            uintptr_t *pte = check_page_helper(info->va, pa2kva(pcb->pgdir));
            if (!info->atsd && ((*pte) & (1 << 6)) == 0 && ((*pte) & (1 << 7)) == 0)
            {
                mem2disk(bid, 8, info->pa, info->va);
                info->atsd = 1;
                info->block = bid;
                info->pte = pte;
                bid += 8;
                *pte &= ~((uint64_t)0x1);
                pcb->pnum --;   
                return ;
            }
            *pte &= ~(uint64_t)(1 << 6);
        }
        p = p->next;
    }
    p = pcb->plist.next;
    while (p != &pcb->plist)
    {
        info = list_entry(p, page_t, list);
        if(!info->atsd)
        {
            uintptr_t *pte = check_page_helper(info->va, pa2kva(pcb->pgdir));
            if (!info->atsd && ((*pte) & (1 << 7)) == 0)
            {
                mem2disk(bid, 8, info->pa, info->va);
                info->atsd = 1;
                info->block = bid;
                info->pte = pte;
                bid += 8;
                *pte &= ~((uint64_t)0x1);
                pcb->pnum --;   
                return ;
            }
            *pte &= ~(uint64_t)(1 << 7);
        }
        p = p->next;
    }
    p = pcb->plist.prev;
    while (p != &pcb->plist)
    {
        info = list_entry(p, page_t, list);
        if(!info->atsd)
        {
            uintptr_t *pte = check_page_helper(info->va, pa2kva(pcb->pgdir));
            if (!info->atsd && (((*pte) & (1 << 7)) == 0))
            {
                mem2disk(bid, 8, info->pa, info->va);
                info->atsd = 1;
                info->block = bid;
                info->pte = pte;
                bid += 8;
                *pte &= ~((uint64_t)0x1);
                pcb->pnum --;   
                return ;
            }
        }
        p = p->prev;
    }
}

void reset_irq_timer()
{
    screen_reflush();
    timer_check();
    sbi_set_timer(get_ticks() + TIMER_INTERVAL);
    do_scheduler();
}

void interrupt_helper(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    handler_t *table = (cause >> 63) ? irq_table : exc_table;
    uint64_t exc_code = cause & 0xffff;
    table[exc_code](regs, stval, cause);
} 

void handle_int(regs_context_t *regs, uint64_t interrupt, uint64_t cause)
{
    reset_irq_timer();
}

void handle_inst_pagefault(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    current_running = (get_current_cpu_id() == 0) ? 
        &current_running_core0 : &current_running_core1;
    uintptr_t kva;
    if ((kva = check_page_helper(stval,
                pa2kva((uintptr_t)((*current_running)->pgdir)))) != 0)
    {
        set_attribute(kva, (1 << 6) | (1 << 7));
        local_flush_tlb_all();
        return ;
    } else {
        printk("> Error: Inst page fault");
        while(1);
    }
}

void handle_load_pagefault(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    current_running = (get_current_cpu_id() == 0) ? 
        &current_running_core0 : &current_running_core1;
    uintptr_t kva;
    if ((kva = check_page_helper(stval,
                pa2kva((uintptr_t)((*current_running)->pgdir)))) != 0)
    {
        set_attribute(kva, (1 << 6));
        local_flush_tlb_all();
        return ;
    }
    if (clock_unblock(*current_running, stval & 0xfffffffffffff000))
    {
        if ((*current_running)->pnum >= 4){
            clock_block((*current_running));
        }
        local_flush_tlb_all();
        return ;
    } 
    if ((*current_running)->pnum >= MAX_PNUM){
        clock_block((*current_running));
    }
    kva = alloc_page_helper((uintptr_t)stval, 
                  pa2kva((uintptr_t)((*current_running)->pgdir)), 1);
    (*current_running)->pnum ++;
    page_t *np = (page_t *)kmalloc(sizeof(page_t));
    np->pa = kva2pa(kva);
    np->va = stval & 0xfffffffffffff000;
    list_add_tail(&(np->list), &((*current_running)->plist));
    local_flush_tlb_all();
}

void handle_store_pagefault(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    current_running = (get_current_cpu_id() == 0) ? 
        &current_running_core0 : &current_running_core1;
    uintptr_t kva;
    if ((kva = check_page_helper(stval,
                pa2kva((uintptr_t)((*current_running)->pgdir)))) != 0)
    {
        set_attribute(kva, (1 << 6) | (1 << 7));
        local_flush_tlb_all();
        return ;
    }
    if (clock_unblock(*current_running, stval & 0xfffffffffffff000))
    {
        if ((*current_running)->pnum >= 40){
            clock_block((*current_running));
        }
        local_flush_tlb_all();
        return ;
    } 
    if ((*current_running)->pnum >= MAX_PNUM){
        clock_block((*current_running));
    }
    kva = alloc_page_helper((uintptr_t)stval, 
                  pa2kva((uintptr_t)((*current_running)->pgdir)), 1);
    (*current_running)->pnum ++;
    page_t *np = (page_t *)kmalloc(sizeof(page_t));
    np->pa = kva2pa(kva);
    np->va = stval & 0xfffffffffffff000;
    list_add_tail(&(np->list), &((*current_running)->plist));
    local_flush_tlb_all();
}

void init_exception()
{
    /* TODO: initialize irq_table and exc_table */
    /* note: handle_int, handle_syscall, handle_other, etc.*/
    int i;
    for (i = 0;i < IRQC_COUNT;i++)
        irq_table[i] = &handle_other;
    for (i = 0;i < EXCC_COUNT;i++)
        exc_table[i] = &handle_other;
    irq_table[IRQC_S_TIMER         ] = &handle_int;
    irq_table[IRQC_S_EXT           ] = &plic_handle_irq;
    exc_table[EXCC_SYSCALL         ] = &handle_syscall;
    exc_table[EXCC_INST_PAGE_FAULT ] = &handle_inst_pagefault;
    exc_table[EXCC_LOAD_PAGE_FAULT ] = &handle_load_pagefault;
    exc_table[EXCC_STORE_PAGE_FAULT] = &handle_store_pagefault;
    setup_exception();
}

void handle_other(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    char* reg_name[32] = {
        "zero "," ra  "," sp  "," gp  "," tp  ",
        " t0  "," t1  "," t2  ","s0/fp"," s1  ",
        " a0  "," a1  "," a2  "," a3  "," a4  ",
        " a5  "," a6  "," a7  "," s2  "," s3  ",
        " s4  "," s5  "," s6  "," s7  "," s8  ",
        " s9  "," s10 "," s11 "," t3  "," t4  ",
        " t5  "," t6  "
    }; 
    for (int i = 0; i < 32; i += 3) {
        for (int j = 0; j < 3 && i + j < 32; ++j) {
            printk("%s : %016lx ",reg_name[i+j], regs->regs[i+j]);
         }
        printk("\n\r");
    } 
    printk("sstatus: 0x%lx sbadaddr: 0x%lx scause: %lu\n\r",
           regs->sstatus, regs->sbadaddr, regs->scause);
    printk("sepc: 0x%lx\n\r", regs->sepc);
        // printk("mhartid: 0x%lx\n\r", get_current_cpu_id());

    uintptr_t fp = regs->regs[8], sp = regs->regs[2];
    printk("[Backtrace]\n\r");
    printk("  addr: %lx sp: %lx fp: %lx\n\r", regs->regs[1] - 4, sp, fp);
    // while (fp < USER_STACK_ADDR && fp > USER_STACK_ADDR - PAGE_SIZE) {
    while (fp > 0x10000) {
        uintptr_t prev_ra = *(uintptr_t*)(fp-8);
        uintptr_t prev_fp = *(uintptr_t*)(fp-16);

        printk("  addr: %lx sp: %lx fp: %lx\n\r", prev_ra - 4, fp, prev_fp);

        fp = prev_fp;
    }

    assert(0);
}
