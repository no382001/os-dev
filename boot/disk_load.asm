disk_load:
	push dx		; store DX on stack so later we can recall how many sectors were request to be read , even if it is altered in the meantime
	
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
