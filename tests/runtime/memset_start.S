  .section .text.init, "ax"
  .globl _start
_start:
  la sp, __stack_top
  la t0, trap
  csrw mtvec, t0
  call test_memset
  j finish
trap:
  li a0, 3
finish:
#ifdef FUXI_SIM
  li t0, 0x10001000
#else
  li t0, 0x100000
  beqz a0, pass
  li a0, 0x13333
  j exit
pass:
  li a0, 0x5555
exit:
#endif
  sw a0, 0(t0)
1: j 1b
