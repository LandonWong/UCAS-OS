#include <asm/sbidef.h>
#include <asm/sbiasm.h>

// #define BOOT_LOADER_SIG_OFFSET 0x1fe
// #define OS_SIZE_LOC (BOOT_LOADER_SIG_OFFSET - 2)
// #define OS_SIZE_LOC 0x1fc
.equ os_size_loc, 0x502001fc

// 1. kernel address (move kernel to here ~)
.equ kernel, 0x50201000

// 2. kernel main address (jmp here to start kernel main!)
.equ kernel_main, 0x50201000

.text
.global main

main:
	// fence on all memory and I/O
	fence
	
    // print a \n to avoid system output error...
	li a0,10
	SBI_CALL SBI_CONSOLE_PUTCHAR

	// task1 call BIOS print string "It's bootblock!
	lui	a0,%hi(msg)
	addi a0,a0,%lo(msg)
	SBI_CALL SBI_CONSOLE_PUTSTR	

	// task2 call BIOS read kernel in SD card and jump to kernel start 0x50200000
	la t0,os_size_loc
	lh t0,0(t0)
	la a0,kernel
	addi a1,t0,0
	addi a2,zero,1
	SBI_CALL SBI_SD_READ
	
	// avoid i cache inconsistent with memory
	fence.i

	// jump to kernel_main
	la t1,kernel_main
	jr t1	

// while(1) --> stop here
stop:
	j stop
	nop

.data

msg: .string "It's a bootloader...\n\r"

