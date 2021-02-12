/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <asm.h>
#include <csr.h>
#include <asm/sbidef.h>
#include <asm/sbiasm.h>

.section ".entry_function","ax"
ENTRY(_start)
  /* Mask all interrupts */
  csrw CSR_SIE, zero
  csrw CSR_SIP, zero

  li t0, SR_FS
  csrc sstatus, t0

  /* Load the global pointer */
  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop

  csrr t0, CSR_MHARTID
  bnez t0, setup_core1
setup_core0:
  la tp, pid0_pcb_core0
  ld sp, pid0_stack_0
  j setup_finish
setup_core1:
  la tp, pid0_pcb_core1
  ld sp, pid0_stack_1
setup_finish:
  call main

loop:
  wfi
  j loop

END(_start)
