  .section  .text.init
  .globl    _start
_start:
  # setup stack
  la    sp, _stack_top
  # jump to machine mode entry
  call  entry
  # switch to supervisor mode
  mret
