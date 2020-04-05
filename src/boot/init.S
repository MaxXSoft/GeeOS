  .section  .text.init, "ax"

  # entry of bootloader
  .globl    _start
_start:
  # setup stack
  la    sp, _stack
  # setup trap handler
  la    t0, _handleTrap
  csrw  mtvec, t0
  # jump to YuLang entry
  j     entry


  # trap handler
  .globl    _handleTrap
_handleTrap:
  # read value of important CSRs
  csrr  a0, mepc
  csrr  a1, mcause
  csrr  a2, mtval
  # jump to YuLang trap handler
  j     handleTrap


  # jump to address
  .globl    jumpToAddr
jumpToAddr:
  jr        a0


  # bootloader stack
  .section  .data
  .align    2
  .zero     512
  .globl    _stack
_stack:
