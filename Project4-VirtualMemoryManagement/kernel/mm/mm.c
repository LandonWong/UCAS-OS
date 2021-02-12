#include <os/list.h>
#include <os/mm.h>
#include <os/sched.h>
#include <pgtable.h>
#include <os/string.h>
#include <assert.h>

ptr_t memCurr = FREEMEM;
ptr_t heapCurr = FREEHEAP;
static LIST_HEAD(freePageList);

typedef struct {
    uint8_t  valid;
    uint8_t  key;
    uint64_t paddr;
    uint64_t vaddr[16];
} shm_page_t;
shm_page_t spage[16];

ptr_t allocPage()
{
    if (!list_empty(&freePageList))
    {
        page_t *new = list_entry(freePageList.next, page_t, list);
        uintptr_t kva = pa2kva(new->pa);
        list_del(freePageList.next);
        return kva;
    }else{
        memCurr += PAGE_SIZE;
        return memCurr;
    }
}

void freePage(ptr_t baseAddr)
{
    /*
    page_t *fp = (page_t *)kmalloc(sizeof(page_t));
    fp->pa = baseAddr;
    list_add_tail(&(fp->list), &freePageList);*/
}

void *kmalloc(size_t size)
{
    ptr_t ret = ROUND(heapCurr, 4);
    heapCurr = ret + size;
    return (void*)ret;
}

uintptr_t shm_page_get(int key)
{
    current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
    uint64_t begin_addr = 0xc00000;
    uint64_t kva;
    int i;
    // Find if exists
    for (i = 0; i < 16; i++)
    {
        if (spage[i].valid && (spage[i].key == key)){
            for (;; begin_addr += 0x1000)
            {
                if (check_page_helper(begin_addr, pa2kva((*current_running)->pgdir)) == 0)
                    break;
            }
            setup_shm_page(begin_addr, spage[i].paddr, pa2kva((*current_running)->pgdir));
            spage[i].vaddr[(*current_running)->pid] = begin_addr;
            return spage[i].vaddr[(*current_running)->pid];
        }
    }
    // Find a free pageframe
    for (;; begin_addr += 0x1000)
    {
        if ((kva = alloc_page_helper(begin_addr, pa2kva((*current_running)->pgdir),1 )) != 0)
            break;
    }
    // Find a free spage control block
    for (i = 0; i < 16; i++)
    {
        if (!spage[i].valid)
            break;
    }
    // Init spage control block
    spage[i].valid = 1;
    spage[i].key = key;
    spage[i].paddr = kva2pa(kva);
    spage[i].vaddr[(*current_running)->pid] = begin_addr;
    return spage[i].vaddr[(*current_running)->pid];
}

void shm_page_dt(uintptr_t addr)
{
    current_running = (get_current_cpu_id() == 0) ? &current_running_core0 : &current_running_core1;
    uint64_t vaddr = addr & 0xfffffffffffff000;
    int i;
    // Find spage cb
    for (i = 0; i < 16; i++)
    {
        if (spage[i].valid && vaddr == spage[i].vaddr[(*current_running)->pid])
            break;
    }
    // Invalid vaddr
    spage[i].vaddr[(*current_running)->pid] = 0;
    // Remove mirror
    free_page_helper(vaddr, (uintptr_t)pa2kva((*current_running)->pgdir));
    // Check valid
    int flag = 0;
    for (int j = 0; j < 16; j++)
    {
        if (spage[i].vaddr[j] != 0)
            flag = 1;
    }
    if (flag == 0)
    {
        spage[i].valid = 0;
        freePage(spage[i].paddr);
    }
}

/* this is used for mapping kernel virtual address into user page table */
void share_pgtable(uintptr_t dest_pgdir, uintptr_t src_pgdir)
{
    // TODO:
}

void setup_shm_page(uintptr_t va, uintptr_t pa, uintptr_t pgdir)
{
    uint64_t vpn[] = {(va >> 12) & ~(~0 << 9),
                      (va >> 21) & ~(~0 << 9),
                      (va >> 30) & ~(~0 << 9)};
    // Find PTE
    PTE *level_2 = (PTE *)pgdir + vpn[2];
    PTE *level_1 = NULL;
    PTE *final_pte = NULL;
    // Find level 2 pgdir
    if (((*level_2) & 0x1) == 0)
    {
        ptr_t newpage = allocPage();
        *level_2 = (kva2pa(newpage) >> 12) << 10;
        set_attribute(level_2, _PAGE_PRESENT);
        level_1 = (PTE *)newpage + vpn[1];
    }
    else
    {
        level_1 = (PTE *)pa2kva(((*level_2) >> 10) << 12) + vpn[1];
    }
    // Find level 1 pgdir
    if (((*level_1) & 0x1) == 0)
    {
        ptr_t newpage = allocPage();
        *level_1 = (kva2pa(newpage) >> 12) << 10;
        set_attribute(level_1, _PAGE_PRESENT);
        final_pte = (PTE *)newpage + vpn[0];
    }
    else
    {
        final_pte = (PTE *)pa2kva(((*level_1) >> 10) << 12) + vpn[0];
    }
    // Generate PTE
    // Set pfn
    set_pfn(final_pte, pa >> 12);
    // Generate flags
    uint64_t pte_flags = _PAGE_READ     | _PAGE_WRITE    | _PAGE_EXEC 
                       | _PAGE_ACCESSED | _PAGE_PRESENT  | _PAGE_DIRTY 
                       | _PAGE_USER;
    set_attribute(final_pte, pte_flags);    
}
/* allocate physical page for `va`, mapping it into `pgdir`,
   return the kernel virtual address for the page.
   */
uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir, int mode)
{
    uint64_t vpn[] = {(va >> 12) & ~(~0 << 9),
                      (va >> 21) & ~(~0 << 9),
                      (va >> 30) & ~(~0 << 9)};
    // Find PTE
    PTE *level_2 = (PTE *)pgdir + vpn[2];
    PTE *level_1 = NULL;
    PTE *final_pte = NULL;
    // Find level 2 pgdir
    if (((*level_2) & 0x1) == 0)
    {
        ptr_t newpage = allocPage();
        *level_2 = (kva2pa(newpage) >> 12) << 10;
        set_attribute(level_2, (mode ? _PAGE_USER : 0) | _PAGE_PRESENT);
        level_1 = (PTE *)newpage + vpn[1];
    }
    else
    {
        level_1 = (PTE *)pa2kva(((*level_2) >> 10) << 12) + vpn[1];
    }
    // Find level 1 pgdir
    if (((*level_1) & 0x1) == 0)
    {
        ptr_t newpage = allocPage();
        *level_1 = (kva2pa(newpage) >> 12) << 10;
        set_attribute(level_1, (mode ? _PAGE_USER : 0) | _PAGE_PRESENT);
        final_pte = (PTE *)newpage + vpn[0];
    }
    else
    {
        final_pte = (PTE *)pa2kva(((*level_1) >> 10) << 12) + vpn[0];
    }
    // Generate PTE
    if ((*final_pte & 0x1) != 0)
        return 0;
    // Set pfn
    ptr_t newpage = allocPage();
    set_pfn(final_pte, (uint64_t)kva2pa(newpage) >> 12);
    // Generate flags
    uint64_t pte_flags = _PAGE_READ     | _PAGE_WRITE    | _PAGE_EXEC 
                       /*| _PAGE_ACCESSED*/ | _PAGE_PRESENT  //| _PAGE_DIRTY 
                       | (mode == 1 ? _PAGE_USER : 0);
    set_attribute(final_pte, pte_flags);
    return (newpage >> 12) << 12;    
}
uintptr_t free_page_helper(uintptr_t va, uintptr_t pgdir)
{
    uint64_t vpn[] = {(va >> 12) & ~(~0 << 9),
                      (va >> 21) & ~(~0 << 9),
                      (va >> 30) & ~(~0 << 9)};
    // Find PTE
    PTE *level_2 = (PTE *)pgdir + vpn[2];
    PTE *level_1 = (PTE *)pa2kva(((*level_2) >> 10) << 12) + vpn[1];
    PTE *final_pte = (PTE *)pa2kva(((*level_1) >> 10) << 12) + vpn[0];
    *final_pte = 0;
}
uintptr_t check_page_helper(uintptr_t va, uintptr_t pgdir)
{
    uint64_t vpn[] = {(va >> 12) & ~(~0 << 9),
                      (va >> 21) & ~(~0 << 9),
                      (va >> 30) & ~(~0 << 9)};
    // Find PTE
    PTE *level_2 = (PTE *)pgdir + vpn[2];
    if (((*level_2) & 0x1) == 0)
        return 0;
    PTE *level_1 = (PTE *)pa2kva(((*level_2) >> 10) << 12) + vpn[1];
    if (((*level_1) & 0x1) == 0)
        return 0;
    PTE *final_pte = (PTE *)pa2kva(((*level_1) >> 10) << 12) + vpn[0];
    if (((*final_pte) & 0x1) == 0)
        return 0;
    return final_pte;
}
