;ensures that we always jump to main instead of serially executing the first function that kernel.c contains
bits 32

%define STACK_SIZE 0x10000
%define STACK_TOP (0x90000 + STACK_SIZE)

_start:
    mov esp, STACK_TOP
    mov ebp, esp

    ; fill the stack w/ magic
    mov edi, esp
    mov ecx, STACK_SIZE
    mov eax, 0xDEADBEEF
    rep stosd

    call setup_paging

    extern kernel_main
    call kernel_main
    jmp $

%include "boot/paging.asm"