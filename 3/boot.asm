;boot sector that boots a C kernel in 32bit protected
org 0x7c00

KERNEL_OFFSET equ 0x1000

;something is fucked up with the kernel offset ld throws a warning and after concatting
;it gets the address of 0x200h (continues right after the magic number at 511-512) instead of 0x1000h

mov [BOOT_DRIVE],dl ;stores dl in boot drive

mov bp,0x9000 ;setup stack
mov sp,bp

mov bx,MSG_REAL_MODE
call print_string

call load_kernel

call switch_to_pm

jmp $

%include "print/print_string.asm"
%include "disk/disk_load.asm" ;no problems 1:1
%include "pm/gdt.asm" ;fixed now 1:1
%include "pm/print_string_pm.asm"
%include "pm/switch_to_pm.asm" ;fixed now 1:1

BITS 16

load_kernel:
	mov bx,MSG_LOAD_KERNEL ;print msg to say we are loading the kernel
	call print_string

	mov bx,KERNEL_OFFSET
	mov dh,15
	mov dl,[BOOT_DRIVE]
	call disk_load

	ret

BITS 32

BEGIN_PM:
	mov ebx,MSG_PROT_MODE ;32bit string to announce we are in protected mode
	call print_string_pm

	call KERNEL_OFFSET

	jmp $ ;hang


;global variables

BOOT_DRIVE		db 0
MSG_REAL_MODE	db "1. Started in 16-bit Real mode", 0
MSG_LOAD_KERNEL	db "2. Loading kernel into memory", 0
MSG_PROT_MODE	db "3. Landed in 32-bit Protected mode", 0

;bootsector padding
times 510-($-$$) db 0
dw 0xaa55

;fill to 4096 0x1000 where the kernel begins
times 4096-($-$$) db 0