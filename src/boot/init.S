  .section  .text.init, "ax"

# entry of bootloader
  .globl    _start
_start:
  # setup stack
  la    sp, _stack
  # setup trap handler
  la    t0, handleTrap
  csrw  mtvec, t0
  # jump to YuLang entry
  j     entry


# bootloader stack
  .section  .data
  .align    2
  .zero     512
  .globl    _stack
_stack:
