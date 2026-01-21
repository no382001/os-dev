OUTPUT_FORMAT("elf32-i386")
ENTRY(_start)

SECTIONS
{
    . = 0x100000;

    .multiboot : ALIGN(4) {
        *(.multiboot)
    }

    . = ALIGN(0x1000);
    .text : {
        *(.text)
    }

    . = ALIGN(0x1000);
    .rodata : {
        *(.rodata)
    }

    . = ALIGN(0x1000);
    .data : {
        *(.data)
    }

    . = ALIGN(0x1000);
    .bss : {
        *(.bss)
        *(COMMON)
    }
    
    _kernel_end = .;
}