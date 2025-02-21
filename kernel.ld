OUTPUT_FORMAT("elf32-i386")
ENTRY(kernel_main)
SECTIONS
{
  . = 0x10000;
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

    . = 0x90000;
    . = ALIGN(16); 
    _stack_bottom = .;      /* Stack starts here */
    . = . + 0x10000;        /* Reserve 64KB for the stack */
    _stack_top = .;  
}