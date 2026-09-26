  .section .text, "ax"
  .balign 4
  .2byte 0
  .globl _start
_start:
  # Exercise full-width instructions and ECALL at a halfword-only entry.
  .option push
  .option norvc
  li a0, 1
  la a1, message
  li a2, message_end - message
  li a7, 64
  ecall
  li a0, 0
  li a7, 93
  ecall
1: j 1b
  .option pop

  .section .rodata
message:
  .ascii "CHECK: halfword entry ran\n"
message_end:
