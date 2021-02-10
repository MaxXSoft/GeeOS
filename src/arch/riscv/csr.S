  .section .text

  .globl  getMstatus
getMstatus:
  csrr  a0, mstatus
  ret

  .globl  setMstatus
setMstatus:
  csrw  mstatus, a0
  ret

  .globl  setMepc
setMepc:
  csrw  mepc, a0
  ret

  .globl  getSstatus
getSstatus:
  csrr  a0, sstatus
  ret

  .globl  setSstatus
setSstatus:
  csrw  sstatus, a0
  ret

  .globl  getSip
getSip:
  csrr  a0, sip
  ret

  .globl  setSip
setSip:
  csrw  sip, a0
  ret

  .globl  getSie
getSie:
  csrr  a0, sie
  ret

  .globl  setSie
setSie:
  csrw  sie, a0
  ret

  .globl  getMie
getMie:
  csrr  a0, mie
  ret

  .globl  setMie
setMie:
  csrw  mie, a0
  ret

  .globl  setSepc
setSepc:
  csrw  sepc, a0
  ret

  .globl  getSepc
getSepc:
  csrr  a0, sepc
  ret

  .globl  getMedeleg
getMedeleg:
  csrr  a0, medeleg
  ret

  .globl  setMedeleg
setMedeleg:
  csrw  medeleg, a0
  ret

  .globl  getMideleg
getMideleg:
  csrr  a0, mideleg
  ret

  .globl  setMideleg
setMideleg:
  csrw  mideleg, a0
  ret

  .globl  setStvec
setStvec:
  csrw  stvec, a0
  ret

  .globl  getStvec
getStvec:
  csrr  a0, stvec
  ret

  .globl  setMtvec
setMtvec:
  csrw  mtvec, a0
  ret

  .globl  setSatp
setSatp:
  csrw  satp, a0
  ret

  .globl  getSatp
getSatp:
  csrr  a0, satp
  ret

  .globl  setSscratch
setSscratch:
  csrw  sscratch, a0
  ret

  .globl  setMscratch
setMscratch:
  csrw  mscratch, a0
  ret

  .globl  getScause
getScause:
  csrr  a0, scause
  ret

  .globl  getStval
getStval:
  csrr  a0, stval
  ret

  .globl  runSfence
runSfence:
  sfence.vma a0, a1
  ret
