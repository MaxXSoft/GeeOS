OUTPUT_ARCH("riscv")
ENTRY(_start)
SECTIONS
{
  . = 0x80000000;
	_geeos_start = .;
	_text_start = .;
  .text : {
	 	*(.text.init)
	 	*(.text)
	 	*(.text*)
	}
	_text_end = .;
	_rodata_start = .;
  .rodata : {
		*(.rodata)
		*(.rodata*)
  }
	_rodata_end = .;
	_data_start = .;
  .data : {
		*(.data)
		*(.data*)
		*(.sdata)
	}
	_data_end = .;
	_bss_start = .;
  .bss : {
		*(.bss)
		*(.bss*)
	}
  . = ALIGN(4) + 0x1000;
  _stack_top = .;
	_bss_end = .;
	_geeos_end = .;
}
