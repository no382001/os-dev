
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; 32 bit protected mode start
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

BITS 32

; jump here from the end of real mode
init_pm:
	mov ax,DATA_SEG ;point to pm segments
	mov ds,ax
	mov ss,ax
	mov es,ax
	mov fs,ax
	mov gs,ax

	call BEGIN_PM

BEGIN_PM:
	mov ebx,MSG_PROT_MODE
	call print_string_pm

	call 0x10000 ; kernel address

	jmp $ ;hang


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; 32 bit protected mode functions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

VIDEO_MEMORY equ 0xb8000
WHITE_ON_BLACK equ 0x0f

print_string_pm:
	pusha
	mov edx,VIDEO_MEMORY

print_string_pm_loop:
	mov al,[ebx]
	mov ah, WHITE_ON_BLACK

	cmp al,0
	je print_string_pm_done

	mov [edx],ax
	add ebx,1
	add edx,2

	jmp print_string_pm_loop

print_string_pm_done:
	popa
	ret

MSG_PROT_MODE	db "3. Landed in 32-bit Protected mode!", 0
MSG_KERNEL_LOAD_FAIL	db "!. Failed to load kernel, check CRC!", 0
