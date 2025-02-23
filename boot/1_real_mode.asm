;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; 16 bit real mode start
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

_bootloader_real_mode_start:
    mov [BOOT_DRIVE],dl ;stores dl in boot drive

    mov bp,0x9000 ;setup stack
    mov sp,bp

    mov bx,MSG_REAL_MODE
    call print_string

    call load_kernel
    
    ; switch to protected mode

    cli

	lgdt [gdt_descriptor] 	;load global descriptor table

	mov eax,cr0 			;set control register
	or eax,0x1
	mov cr0,eax

	jmp CODE_SEG:init_pm 	;flush cache and far jump to 32bit code

    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; 16 bit real mode functions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

BITS 16

; print a string in real mode, expects zero delimited address in bx
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
    mov al,0x0d     ;carriage return
    int 0x10
    mov al,0x0a     ;line feed
    int 0x10

    popa            ; pop our preserved register values back from stack
    ret

;;;;;;;;;;

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

;;;;;;;;;;

disk_load:
	push dx		; store DX on stack so later we can recall how many sectors were requested to be read, even if it is altered in the meantime
	
	mov ah,0x02	; BIOS read sector function
	mov al,dh	; read DH sectors
	mov ch,0x00	; select cylinder 0
	mov dh,0x00	; select head 0
	mov cl,0x02	; start reading from second sector ( i.e. after the boot sector )
	
	int 0x13	; BIOS interrupt
	
	jc disk_error ; jump if error ( i.e. carry flag set )

	pop dx
	cmp dh,al	; if AL ( sectors read ) != DH ( sectors expected )
	jne sector_error
	ret 
	
disk_error:
	mov bx, DISK_ERROR_MSG
	call print_string
	jmp $

sector_error:
    mov bx, SECTOR_ERROR
    call print_string
	jmp $

DISK_ERROR_MSG db "Disk read error !" , 0
SECTOR_ERROR: db "Incorrect number of sectors read", 0
BOOT_DRIVE		db 0
MSG_BOOT_DRIVE  db "Boot drive detected", 0
MSG_REAL_MODE	db "1. Started in 16-bit Real mode", 0
MSG_LOAD_KERNEL	db "2. Loading kernel into memory", 0