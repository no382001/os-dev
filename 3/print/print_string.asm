print_string:
    pusha           ; preserve our general purpose registers on the stack
    mov ah, 0x0e    ; teletype function
.work:
    mov al, [bx]    ; move the value pointed at by bx to al
    cmp al, 0       ; check for null termination
    je .done        ; jump to finish if null
    int 0x10        ; fire our interrupt: int 10h / AH=0E
    add bx, 1       ; increment bx pointer by one
    jmp .work       ; loop back
.done:
    popa            ; pop our preserved register values back from stack
    ret