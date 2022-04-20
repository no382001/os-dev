# bootloader
**following this paper:**
*Writing a Simple Operating System from Scratch by Nick Blundell
School of Computer Science, University of Birmingham, UK
Draft: December 2, 2010
Copyright c 2009–2010 Nick Blundell*

**using virtualbox**
following this source: https://stackoverflow.com/questions/43566542/virtualbox-no-bootable-medium-found

	

	    ;//boot.asm
    	
    	BITS 16
    	ORG 0x7C00
    
        xor ax, ax
        mov ds, ax
        mov ss, ax       ; Stack below bootloader
        mov sp, 0x7c00
    
        mov ax, 0xb800   ; Video segment b800
        mov es, ax
    
        ; Print Hello with white on light magenta
        mov word [es:0x0], 0x57 << 8 | 'H'
        mov word [es:0x2], 0x57 << 8 | 'e'
        mov word [es:0x4], 0x57 << 8 | 'l'
        mov word [es:0x6], 0x57 << 8 | 'l'
        mov word [es:0x8], 0x57 << 8 | 'o'
    
        ; End with infinite loop
        cli
	    endloop:
        hlt
        jmp endloop
    
	    ; Fill out to 510 bytes and add boot signature
	    times 510 - ($ - $$) db 0
	    dw 0xAA55            ; add boot signature at the end of bootloader


create bootloader.bin with nasm compiler
```
nasm -f bin boot.asm -o boot.bin
```
create a raw disk image with dd (2MB in this case)

	dd if=/dev/zero of=1.raw bs=1024 count=2048
place the binary to the beginning of the disk image without turnicating the image

```
dd if=boot.bin of=1.raw conv=notrunc
```
create a vdi file from the raw file
			
	VBoxManage convertfromraw 1.raw 1.vdi --format VDI

this vdi file will boot in VirtualBox
