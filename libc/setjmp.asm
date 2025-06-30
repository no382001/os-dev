; setjmp.asm - NASM implementation for 32-bit x86
; 
; jmp_buf structure layout:
; 0:  ebx
; 4:  esi  
; 8:  edi
; 12: ebp
; 16: esp
; 20: eip (return address)

section .text

global setjmp
setjmp:
    mov eax, [esp + 4]      ; get jmp_buf pointer
    
    mov [eax + 0], ebx
    mov [eax + 4], esi  
    mov [eax + 8], edi
    mov [eax + 12], ebp
    
    mov [eax + 16], esp
    
    mov edx, [esp]          ; get return address from top of stack
    mov [eax + 20], edx     ; save return address
    
    ; return 0 on direct call
    xor eax, eax
    ret

global longjmp  
longjmp:
    mov eax, [esp + 4]      ; get jmp_buf pointer
    mov edx, [esp + 8]      ; get return value
    
    ; Ensure return value is not 0
    test edx, edx
    jnz .non_zero
    inc edx                 ; if val is 0, make it 1
.non_zero:

    mov ecx, [eax + 20]
    
    mov ebx, [eax + 0]
    mov esi, [eax + 4]
    mov edi, [eax + 8]
    mov ebp, [eax + 12]
    
    ; restore stack pointer
    mov esp, [eax + 16]
    
    ; set return value
    mov eax, edx
    
    ; jump to saved return address
    jmp ecx