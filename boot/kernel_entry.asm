;ensures that we always jump to main instead of serially executing the first function that kernel.c contains
bits 32

%define STACK_BOTTOM 0x60000
%define STACK_SIZE 0x40000 ; 256kb
%define STACK_TOP (STACK_BOTTOM + STACK_SIZE)

_start:
    mov esp, STACK_TOP
    mov ebp, esp

    ; fill the stack w/ magic
    mov edi, STACK_TOP - STACK_SIZE
    mov ecx, STACK_SIZE / 4
    mov eax, 0xDEADBEEF
    rep stosd

    extern kernel_main
    call kernel_main
    jmp $