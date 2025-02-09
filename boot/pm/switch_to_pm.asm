BITS 16

switch_to_pm:
	cli

	lgdt [gdt_descriptor] 	;load global descriptor table

	mov eax,cr0 			;set control register
	or eax,0x1
	mov cr0,eax

	jmp CODE_SEG:init_pm 	;flush cache and far jump to 32bit code

BITS 32

init_pm:
	
	mov ax,DATA_SEG 		;point to pm segments
	mov ds,ax
	mov ss,ax
	mov es,ax
	mov fs,ax
	mov gs,ax

	mov ebp,0x90000 		;update stack, to the top of free space
	mov esp,ebp

	call BEGIN_PM