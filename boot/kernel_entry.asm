;ensures that we always jump to main instead of serially executing the first function that kernel.c contains
bits 32
_start:
    call setup_paging
    
    extern kernel_main
    call kernel_main
    jmp $

%include "boot/paging.asm"