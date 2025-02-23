OUTPUT_FORMAT("elf32-i386")
ENTRY(kernel_main)
SECTIONS
{
  . = 0x10000;
  _kernel_start = .;
  .text : {
      *(.text)
  }
  .rodata : {
      *(.rodata)
  }
  .data : {
      *(.data)
  }
  . = ALIGN(16);
  .bss : {
      *(.bss)
  }
  _kernel_end = .;
  
  _stack_bottom = 0x60000;
  _stack_top = 0xA0000;
}