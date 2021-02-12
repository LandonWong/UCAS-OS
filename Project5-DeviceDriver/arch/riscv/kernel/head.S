/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <asm.h>
#include <csr.h>

/* maybe you need another stack for booting? */
.equ boot_stack_top_base, 0x50202000

.section ".entry_function","ax"
ENTRY(_start)
  /* TODO: */
  /* setup C environment for boot.c */

  // Mask all interrupt
  csrw CSR_SIE, zero
  csrw CSR_SIP, zero
  
  // Disable FPU in kernel space
  li t0, SR_FS
  csrc sstatus, t0

  la sp, boot_stack_top_base

  //sd a1, (a0)
  //la a0, riscv_dtb
  csrr a0, CSR_MHARTID 

  /* this should never return */
  call boot_kernel

loop:
  wfi
  j loop

END(_start)
