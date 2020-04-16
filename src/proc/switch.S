.equ XLENB, 4

.macro LOAD a1, a2
  lw \a1, \a2*XLENB(sp)
.endm

.macro STORE a1, a2
  sw \a1, \a2*XLENB(sp)
.endm

  .section  .text
  .globl    switchTo
switchTo:
  addi  sp, sp, -14*XLENB
  sw    sp, 0(a0)
  STORE ra, 0
  STORE s0, 2
  STORE s1, 3
  STORE s2, 4
  STORE s3, 5
  STORE s4, 6
  STORE s5, 7
  STORE s6, 8
  STORE s7, 9
  STORE s8, 10
  STORE s9, 11
  STORE s10, 12
  STORE s11, 13
  csrr  s11, satp
  STORE s11, 1

  lw    sp, 0(a1)
  LOAD  s11, 1
  csrw  satp, s11
  sfence.vma
  LOAD  ra, 0
  LOAD  s0, 2
  LOAD  s1, 3
  LOAD  s2, 4
  LOAD  s3, 5
  LOAD  s4, 6
  LOAD  s5, 7
  LOAD  s6, 8
  LOAD  s7, 9
  LOAD  s8, 10
  LOAD  s9, 11
  LOAD  s10, 12
  LOAD  s11, 13
  addi  sp, sp, 14*XLENB

  sw    zero, 0(a1)
  ret
