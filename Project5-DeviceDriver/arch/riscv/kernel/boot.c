/* RISC-V kernel boot stage */
#include <context.h>
#include <os/elf.h>
#include <pgtable.h>
#include <sbi.h>
#define PGDIR 0x5e000000
typedef void (*kernel_entry_t)(unsigned long, uintptr_t);

extern unsigned char _elf_main[];
extern unsigned _length_main;

/********* setup memory mapping ***********/
uintptr_t alloc_page()
{
    PTE *pgdir = PGDIR_PA;
    // Alloc all 512 level2 pgdir
    for (int i = 0; i < 512; i++)
        *(pgdir + i) = ((PGDIR_PA >> 12) + i + 1) << 10;
}

// using 2MB large page
void map_page(uint64_t va, uint64_t pa, PTE *pgdir)
{
    // Get vpn[2] and vpn[1]
    uint64_t vpn_2 = (va >> 30) & ~(~0 << 9);
    uint64_t vpn_1 = (va >> 21) & ~(~0 << 9);
    // Get ppn: ppn[2:0]
    uint64_t pfn = pa >> 12;
    // Find 2nd level pgdir address
    PTE *level_2 = pgdir + vpn_2;
    // Mark as valid
    set_attribute(level_2, _PAGE_PRESENT);
    // Find PTE to be mapped
    PTE *map_pte = (PTE *)get_pa(*level_2) + vpn_1;
    // Generate flags
    uint64_t pte_flags = _PAGE_READ     | _PAGE_WRITE    | _PAGE_EXEC 
                       | _PAGE_ACCESSED | _PAGE_PRESENT  | _PAGE_DIRTY ;
    // Write to PTE
    set_attribute(map_pte, pte_flags);
    set_pfn(map_pte, pfn);
}

void enable_vm()
{
    local_flush_tlb_all();
    set_satp(SATP_MODE_SV39, BOOT_ASID, PGDIR_PA >> 12);
}

/* Sv-39 mode
 * 0x0000_0000_0000_0000-0x0000_003f_ffff_ffff is for user mode
 * 0xffff_ffc0_0000_0000-0xffff_ffff_ffff_ffff is for kernel mode
 */
void setup_vm()
{
    clear_pgdir(PGDIR_PA);
    alloc_page();
    // map kernel virtual address(kva) to kernel physical
    // address(kpa) kva = kpa + 0xffff_ffc0_0000_0000 use 2MB page,
    uint64_t kva = 0x50200000;
    uint64_t kpa = 0x50200000;
    // Map kernel itself for boot.c
    // 0x50200000 ~ 0x50400000
    map_page(kva, kpa, PGDIR_PA);
    // map all physical memory
    for (uint64_t addr = 0x50200000; addr < 0x5ef000000; addr += 0x200000)
        map_page(pa2kva(addr), addr, PGDIR_PA);
    // enable virtual memory
    enable_vm();    
}

uintptr_t directmap(uintptr_t kva, uintptr_t pgdir)
{
    return kva;
}

kernel_entry_t start_kernel = NULL;

/*********** start here **************/
int boot_kernel(unsigned long mhartid, uintptr_t riscv_dtb)
{
    if (mhartid == 0) {
        setup_vm();
        start_kernel = (kernel_entry_t)load_elf(_elf_main, _length_main,
                                                PGDIR_PA , directmap);
    } else {
        enable_vm();
    }
    start_kernel(mhartid, riscv_dtb);
    return 0;
}
