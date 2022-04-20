BITS 16                     ; set to 16bits 
ORG 0x7C00                  ; offset to bootsector
xor ax, ax                  ; zero registers
mov ds, ax
mov ss, ax
mov sp, 0x7c00              ; point stack to 0x7c00 where the code begins
cli
endloop:                    ;infloop
    hlt
    jmp endloop
times 510 - ($ - $$) db 0   ; fill with 510 zeros
dw 0xAA55                   ; add magic number to the end