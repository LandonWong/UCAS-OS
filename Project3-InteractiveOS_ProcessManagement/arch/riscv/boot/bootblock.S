#include <asm/sbidef.h>
#include <asm/sbiasm.h>
#include <csr.h>
// 1. kernel address (move kernel to here ~) 
.equ kernel, 0x50201000
// 2. kernel main address (jmp here to start kernel main!) 
.equ kernel_main, 0x50201000
// 3. os size location (get the size of your OS here) 
.equ os_size_loc, 0x502001fc 
.text 
.global main 
main:
	// fence on all memory and I/O
	fence

    // a0 is mhartid, a1 is the pointer of fdt
	move s0,a0
	move s1,a1
    
    /* Load the global pointer */
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop
  
	// The core whose mhartid=0 is selected to boot the kernel.
	// Other cores should wait until the kernels initialization
	// has been finished.
    move a0, s0
	bnez a0, secondary

	// your code 
    la a0, msg
    SBI_CALL SBI_CONSOLE_PUTSTR 
    
    la t0,os_size_loc
    lh t0,0(t0)
    la a0,kernel
    mv a1, t0
    addi a2,zero,1
    SBI_CALL SBI_SD_READ

    fence.i
    
    la t1,kernel_main
    jr t1
    
secondary:
	/* TODO:
	 * 1. Mask all interrupts
	 * 2. let stvec pointer to kernel_main
	 * 3. enable software interrupt for ipi
	 */ 
	fence
    
    // Mark all interrupt
    csrw CSR_SIE, zero

    // Set stvec to kernel entry
    la t0, kernel_main
    csrw CSR_STVEC, t0
    
    // Enable supervisor software interrupt (ipi)
    li t0, SIE_SSIE
    csrs CSR_SIE, t0
    
    li t0, SR_SIE
    csrw CSR_SSTATUS, t0
    
    la a0, msg2
    SBI_CALL SBI_CONSOLE_PUTSTR 
    fence.i

wait_for_wakeup:
	wfi
	j wait_for_wakeup 

stop:
	j stop
	nop

.data
msg : .string "INFO: CORE 0 STARTED\n\r"
msg2: .string "INFO: CORE 1 STARTED\n\r"
