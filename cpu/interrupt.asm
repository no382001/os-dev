extern isr_handler
extern irq_handler
extern enter_interrupt_context
extern exit_interrupt_context

%macro ISR_NOERR 1
    global isr%1
isr%1:
    push byte 0 ; placeholder, so that stack would look as if it's an exception with err code
    push %1
    jmp isr_common_stub
%endmacro

%macro ISR_ERR 1
    global isr%1
isr%1:
    push %1
    jmp isr_common_stub
%endmacro

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR 8
ISR_NOERR 9
ISR_ERR 10
ISR_ERR 11
ISR_ERR 12
ISR_ERR 13
ISR_ERR 14
ISR_NOERR 15
ISR_NOERR 16
ISR_NOERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31


isr_common_stub:
    pusha ; push eax, ebx, ecx, edx, esi, edi, ebp, esp
    mov ax, ds
    push eax ; save ds
    
    mov ax, 0x10 ; load kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call enter_interrupt_context
    call isr_handler
    call exit_interrupt_context

    pop ebx ; restore original data segment
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx

    popa
    add esp, 0x8 ; rebalance stack(pop err code and exception number)
    ; i shouldnt need cli/sti here, its redundant, i read somewhere...
    iret

%macro IRQ 2
    global irq%1
irq%1:
    push byte 0 ; always push a dummy err code so that we can reuse the register_t struct
    push byte %2
    jmp irq_common_stub
%endmacro

IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47


irq_common_stub:
    pusha ; push eax, ebx, ecx, edx, esi, edi, ebp, esp
    mov ax, ds
    push eax ; save ds

    mov ax, 0x10 ; load kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call enter_interrupt_context
    call irq_handler
    call exit_interrupt_context

    pop ebx ; restore original data segment
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx

    popa
    add esp, 0x8 ; rebalance stack(pop err code and exception number)

    iret