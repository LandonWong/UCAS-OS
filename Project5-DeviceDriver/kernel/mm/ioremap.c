#include <os/ioremap.h>
#include <os/list.h>
#include <os/mm.h>
#include <pgtable.h>
#include <type.h>

// maybe you can map it to IO_ADDR_START ?
static uintptr_t io_base = IO_ADDR_START;

void *ioremap(unsigned long phys_addr, unsigned long size)
{
    // map phys_addr to a virtual address
    // then return the virtual address
    uintptr_t pgdir = 0xffffffc05e000000;   
    int flag = 0;
    uint64_t vaddr = NULL;
    while (size != 0)
    {
        uint64_t va = io_base;
        uint64_t vpn[] = {(va >> 12) & ~(~0 << 9),
                          (va >> 21) & ~(~0 << 9),
                          (va >> 30) & ~(~0 << 9)};
        PTE *lv2 = (PTE *)pgdir + vpn[2];
        PTE *lv1 = NULL;
        PTE *pte = NULL;
        if (((*lv2) & 0x1) == 0)
        {
            ptr_t newpage = allocPage();
            *lv2 = (kva2pa(newpage) >> 12) << 10;
            set_attribute(lv2, _PAGE_PRESENT);
            lv1 = (PTE *)newpage + vpn[1];
        }
        else
        {
            lv1 = (PTE *)pa2kva(((*lv2) >> 10) << 12) + vpn[1];
        }
        if (((*lv1) & 0x1) == 0)
        {
            ptr_t newpage = allocPage();
            *lv1 = (kva2pa(newpage) >> 12) << 10;
            set_attribute(lv1, _PAGE_PRESENT);
            pte = (PTE *)newpage + vpn[0];
        }
        else
        {
            pte = (PTE *)pa2kva(((*lv1) >> 10) << 12) + vpn[0];
        }
        set_pfn(pte, phys_addr >> 12);
        uint64_t pte_flags = _PAGE_READ     | _PAGE_WRITE    | _PAGE_EXEC 
                           | _PAGE_ACCESSED | _PAGE_PRESENT  | _PAGE_DIRTY;
        set_attribute(pte, pte_flags);
        if (!flag)
        {
            flag = 1;
            vaddr = io_base;
        }
        io_base = (uint64_t)io_base + PAGE_SIZE;
        phys_addr = (uint64_t)phys_addr + PAGE_SIZE;
        size -= PAGE_SIZE;
    }
    local_flush_tlb_all();
    return vaddr;
}

void iounmap(void *io_addr)
{
    // TODO: a very naive iounmap() is OK
    // maybe no one would call this function?
    // *pte = 0;
}
