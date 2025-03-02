OUTPUT_FORMAT("elf32-i386")
ENTRY(_start)

SECTIONS
{
  . = 0x10000;
  
  . = ALIGN(0x1000);
  .text : ALIGN(0x1000) {
      *(.text)
  }

  . = ALIGN(0x1000);
  .rodata : ALIGN(0x1000) {
      *(.rodata)
  }

  . = ALIGN(0x1000);
  .data : ALIGN(0x1000) {
      *(.data)
  }

  . = ALIGN(0x1000);
  .bss : ALIGN(0x1000) {
      *(.bss)
      *(COMMON)
  }
  _kernel_end = .;
}
