  # switch to higher virtual address space
  #   a0: token for SATP
  .section  .text
  .globl    switchToVm
switchToVm:
  csrw  satp, a0
  sfence.vma
  lui   a0, %hi(_switchToVm_ret)
  addi  a0, a0, %lo(_switchToVm_ret)
  jr    a0
_switchToVm_ret:
  ret


  # root page table
  .section  .data
  .align    12
  .globl    _root_table
_root_table:
  .zero     4 * 1024
