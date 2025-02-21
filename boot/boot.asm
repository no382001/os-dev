;boot sector that boots a C kernel in 32bit protected
org 0x7c00

; kernel_load does this manually
KERNEL_OFFSET equ 0x10000

mov [BOOT_DRIVE],dl ;stores dl in boot drive

mov bp,0x9000 ;setup stack
mov sp,bp

mov bx,MSG_REAL_MODE
call print_string

call load_kernel

call switch_to_pm

jmp $

%include "boot/print_string.asm"
%include "boot/disk_load.asm"
%include "boot/pm/gdt.asm"
%include "boot/pm/print_string_pm.asm"
%include "boot/pm/switch_to_pm.asm"

BITS 16

load_kernel:
	mov bx,MSG_LOAD_KERNEL
	call print_string

	mov ax, 0x1000      ; set segment = 0x1000 (0x10000 / 16)
	mov es, ax          ; ES now points to 0x10000
	mov bx, 0x0000      ; offset in segment
	mov dh, 80
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
MSG_BOOT_DRIVE  db "Boot drive detected", 0
MSG_REAL_MODE	db "1. Started in 16-bit Real mode", 0
MSG_LOAD_KERNEL	db "2. Loading kernel into memory", 0
MSG_PROT_MODE	db "3. Landed in 32-bit Protected mode!", 0
MSG_KERNEL_LOAD_FAIL	db "!. Failed to load kernel, check CRC!", 0

;bootsector padding
times 510-($-$$) db 0
dw 0xaa55

; im not sure why we need this?
;fill to 4096 0x1000 where the kernel begins
;times 4096-($-$$) db 0