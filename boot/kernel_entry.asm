;ensures that we always jump to main instead of serially executing the first function that kernel.c contains
bits 32
_start:
    mov esp, 0x90000
    mov ebp, esp

    mov edi, esp
    mov ecx, 0x8000     ; 16KB stack (adjust based on your stack size)
    mov eax, 0xDEADBEEF
    rep stosd

    call setup_paging

    extern kernel_main
    call kernel_main
    jmp $

%include "boot/paging.asm"