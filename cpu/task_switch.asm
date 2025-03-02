
global switch_stack_and_jump
switch_stack_and_jump:
    mov ebx, [esp + 8] ; second argument, the function pointer
    mov eax, [esp + 4] ; first argument, a location to build a stack
    mov esp, eax ; set the stack pointer to the location
    mov ebp, esp ; by setting the EBP to the ESP, we configure a new stack
    sti ; enable interrupts
    jmp ebx ; jump to the function pointer