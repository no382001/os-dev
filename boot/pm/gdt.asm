gdt_start:

gdt_null:	;mandatory null descriptor
	dd 0x0
	dd 0x0

gdt_code:	;code segment descriptor
	;base=0x0, limit 0xfffff
	;1st flag: (present)1 (priviledge)00 (descriptor type)1 -> 1001b
	;type flags: (granularity)1 (32-bit default)1 (64-bit seg)0 (AVL)0 -> 1100b
	dw 0xffff ;limit (0-15)b
	dw 0x0 ;base (0-15)b
	db 0x0 ;base (16-23)b
	db 10011010b ;1st flags, type flags
	db 11001111b ;2nd flags, limit (16-19)b
	db 0x0	;base (24-31)b

gdt_data:
	;same as code seg except type
	; type f: (code)0 (expand down)0 (writable)1 (accessed)0 -> 0010b
	dw 0xffff ;limit (0-15)b
	dw 0x0 ;base (0-15)b
	db 0x0 ;base (16-23)b
	db 10010010b ;1st flags, type flags
	db 11001111b ;2nd flags, limit (16-19)b
	db 0x0	;base (24-31)b

gdt_end:	;so assembler can calculate the size of the GDT for the GDT descpritor

gdt_descriptor:
	dw gdt_end - gdt_start - 1
	dd gdt_start

;handy constants
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start