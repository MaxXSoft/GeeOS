.equ XLENB, 4

.macro LOAD reg, mem
  lw \reg, \mem
.endm

.macro STORE reg, mem
  sw \reg, \mem
.endm

  .section  .text
  .globl    switchTo
switchTo:
  addi  sp, sp, (-XLENB*14)
  STORE sp, 0(a0)
  STORE ra, 0*XLENB(sp)
  STORE s0, 2*XLENB(sp)
  STORE s1, 3*XLENB(sp)
  STORE s2, 4*XLENB(sp)
  STORE s3, 5*XLENB(sp)
  STORE s4, 6*XLENB(sp)
  STORE s5, 7*XLENB(sp)
  STORE s6, 8*XLENB(sp)
  STORE s7, 9*XLENB(sp)
  STORE s8, 10*XLENB(sp)
  STORE s9, 11*XLENB(sp)
  STORE s10, 12*XLENB(sp)
  STORE s11, 13*XLENB(sp)
  csrr  s11, satp
  STORE s11, 1*XLENB(sp)

  LOAD  sp, 0(a1)
  LOAD  s11, 1*XLENB(sp)
  csrw  satp, s11
  sfence.vma
  LOAD  ra, 0*XLENB(sp)
  LOAD  s0, 2*XLENB(sp)
  LOAD  s1, 3*XLENB(sp)
  LOAD  s2, 4*XLENB(sp)
  LOAD  s3, 5*XLENB(sp)
  LOAD  s4, 6*XLENB(sp)
  LOAD  s5, 7*XLENB(sp)
  LOAD  s6, 8*XLENB(sp)
  LOAD  s7, 9*XLENB(sp)
  LOAD  s8, 10*XLENB(sp)
  LOAD  s9, 11*XLENB(sp)
  LOAD  s10, 12*XLENB(sp)
  LOAD  s11, 13*XLENB(sp)
  addi  sp, sp, (XLENB*14)

  STORE zero, 0(a1)
  ret
