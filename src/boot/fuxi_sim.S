  # The simulator loads the kernel ELF into RAM before releasing reset.
  # Load this raw ROM image at 0x200, the fixed Fuxi reset PC.
  .section .text.init, "ax"
  .globl _start
_start:
  li t0, 0x80000000
  jr t0
