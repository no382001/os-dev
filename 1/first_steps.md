**following this paper:**
*Writing a Simple Operating System from Scratch by Nick Blundell
School of Computer Science, University of Birmingham, UK
Draft: December 2, 2010
Copyright c 2009–2010 Nick Blundell*
source: https://www.cs.bham.ac.uk/~exr/lectures/opsys/10_11/lectures/os-dev.pdf

**using virtualbox**
building a vdi following this: https://stackoverflow.com/questions/43566542/virtualbox-no-bootable-medium-found
and this source: https://stackoverflow.com/questions/32174806/does-the-bios-copy-the-512-byte-bootloader-to-0x7c00

first create a *boot.asm* according chapter 2.2 of the main paper

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

*(its better if you place the next codes in a bashscript)*

then compile *boot.asm* with **nasm** to a binary
```
nasm -f bin boot.asm -o boot.bin
```
create a raw disk image with **dd** (2MB in this case)

	dd if=/dev/zero of=1.raw bs=1024 count=2048
**dd** the binary to the beginning of the disk image without turncating the image

```
dd if=boot.bin of=1.raw conv=notrunc
```
create a *.vdi* file from the raw disk image
			
	VBoxManage convertfromraw 1.raw 1.vdi --format VDI

this *vdi* file will boot in VirtualBox and hang on a black screen because of the infinite loop

