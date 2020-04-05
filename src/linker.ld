OUTPUT_ARCH("riscv")
ENTRY(_start)
SECTIONS
{
  . = 0x80000000;
	_geeos_start = .;
  .text : {
	 	*(.text.init)
	 	*(.text)
	 	*(.text*)
	}
	_text_end = .;
  .rodata : {
		*(.rodata)
		*(.rodata*)
  }
  .data : {
		*(.data)
		*(.data*)
		*(.sdata)
	}
  .bss : {
		*(.bss)
		*(.bss*)
	}
  . = ALIGN(4) + 0x1000;
  _stack_top = .;
	_geeos_end = .;
}
