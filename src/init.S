  .section  .text.init
  .globl    _start
_start:
  # setup stack
  la    sp, _stack_top
  # jump to machine mode entry (in 'entry.yu')
  call  entry


  # root page table
  .section  .data
  .align    12
  .globl    _root_table
_root_table:
  .zero     4 * 1024
