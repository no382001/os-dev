OUTPUT_FORMAT("elf32-i386")
ENTRY(kernel_main)

SECTIONS
{
  /* Kernel starts at 0x10000 */
  . = 0x10000;
  
  /* Align .text section to 4KB */
  . = ALIGN(0x1000);
  .text : ALIGN(0x1000) {
      *(.text)
  }

  /* Align .rodata section to 4KB */
  . = ALIGN(0x1000);
  .rodata : ALIGN(0x1000) {
      *(.rodata)
  }

  /* Align .data section to 4KB */
  . = ALIGN(0x1000);
  .data : ALIGN(0x1000) {
      *(.data)
  }

  /* Align .bss section to 4KB */
  . = ALIGN(0x1000);
  .bss : ALIGN(0x1000) {
      *(.bss)
      *(COMMON)  /* Ensure uninitialized globals go here */
  }
}
