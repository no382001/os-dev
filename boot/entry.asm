bits 32

section .multiboot
align 4
    dd 0x1BADB002              ; magic
    dd 0x07                    ; flags: align + meminfo + video
    dd -(0x1BADB002 + 0x07)    ; checksum
    
    ; address fields (not used with ELF)
    dd 0, 0, 0, 0, 0
    
    ; video mode request
    dd 0                       ; mode_type: 0 = linear graphics
    dd 800                     ; width
    dd 600                     ; height
    dd 32                      ; bpp

section .text
global _start
extern kernel_main

_start:
    mov esp, stack_top
    
    ; ebx = multiboot info struct, pass to kernel
    push ebx
    ; eax = magic number
    push eax
    
    call kernel_main
    
.hang:
    cli
    hlt
    jmp .hang

section .bss
align 16
stack_bottom:
    resb 0x40000              ; 256KB stack
stack_top: