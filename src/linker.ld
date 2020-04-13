OUTPUT_ARCH("riscv")
ENTRY(_start)
SECTIONS
{
  . = 0x80000000;
  _geeos_start = .;
  .text : {
    _text_start = .;
    *(.text.init)
    *(.text)
    *(.text*)
    . = ALIGN(4K);
    _text_end = .;
  }
  .rodata : {
    _rodata_start = .;
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4K);
    _rodata_end = .;
  }
  .data : {
    _data_start = .;
    *(.data)
    *(.data*)
    *(.sdata)
    _data_end = .;
  }
  .bss : {
    _bss_start = .;
    *(.bss)
    *(.sbss*)
    _bss_end = .;
  }
  _stack_start = .;
  . = ALIGN(4) + 0x1000;
  _stack_top = .;
  _stack_end = .;
  _geeos_end = .;
}
