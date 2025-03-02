bits 32
section .text ; mboot
        align 4
        dd 0x1BADB002
        dd 0x00
        dd - (0x1BADB002 + 0x00)

%define STACK_BOTTOM 0x60000
%define STACK_SIZE 0x40000 ; 256kb
%define STACK_TOP (STACK_BOTTOM + STACK_SIZE)

extern _start
extern kernel_main
_start:
    mov esp, STACK_TOP
    mov ebp, esp

    ; fill the stack xDEADBEEF
    mov edi, STACK_TOP - STACK_SIZE
    mov ecx, STACK_SIZE / 4
    mov eax, 0xDEADBEEF
    rep stosd

    call kernel_main
    jmp $