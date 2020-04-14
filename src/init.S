  .section  .text.init
  .globl    _start
_start:
  # setup stack
  la    sp, _stack_top
  # jump to machine mode entry (in 'entry.yu')
  call  entry


  # first user program
  .section  .data
  .globl    _user_img_start
_user_img_start:
  .incbin   "usr/hello"
  .globl    _user_img_end
_user_img_end:
