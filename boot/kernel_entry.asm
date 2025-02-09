;ensures that we always jump to main instead of serially executing the first function that kernel.c contains
bits 32
extern main
call main
jmp $