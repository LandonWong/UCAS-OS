#include <asm.h>
#include <csr.h>

ENTRY(setup_exception)
  /* TODO: 
   * save exception_handler_entry into STVEC
   * enable global exceptions */
  la s0, exception_handler_entry
  csrw stvec, s0
  li s0, SR_SIE
  csrw sstatus, s0
  li s0, SIE_SSIE | SIE_STIE | SIE_SEIE
  csrw sie, s0
  jr ra
ENDPROC(setup_exception)
