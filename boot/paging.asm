%define PAGE_DIR_ADDR         0x0020000
%define PAGE_TABLE_IDENTITY   0x0030000
%define PAGE_TABLE_KERNEL     0x0040000

%define KERNEL_PHYS_START     0x00001000
%define KERNEL_VIRT_START     0xC0010000

%define KERNEL_NUM_PAGES      64    ; Example only, adjust as needed

bits 32

global setup_paging
setup_paging:
    mov edi, PAGE_DIR_ADDR
    mov ecx, 4096 * 3       ; 3 pages of 4096 bytes
    xor eax, eax
    rep stosb               ; zero them all

    ; link the page directory to the identity page table
    mov eax, PAGE_TABLE_IDENTITY
    or eax, 0x03            ; PRESENT + RW
    mov [PAGE_DIR_ADDR + (0 * 4)], eax

    ; identity page table entries.
    mov edi, PAGE_TABLE_IDENTITY
    xor esi, esi            ; start physical = 0
    mov ecx, 256            ; number of pages to identity-map (1 MB)
id_map_loop:
    mov eax, esi            ; physical address
    or eax, 0x03            ; PRESENT + RW
    mov [edi], eax
    add edi, 4
    add esi, 0x1000         ; next 4KiB
    dec ecx
    jnz id_map_loop

    ; link PDE #768 => kernel page table
    mov eax, PAGE_TABLE_KERNEL
    or eax, 0x03            ; PRESENT + RW
    mov [PAGE_DIR_ADDR + (768 * 4)], eax

    ; fill out kernel's higher-half mapping:
    ;    KERNEL_PHYS_START -> KERNEL_VIRT_START
    mov edi, PAGE_TABLE_KERNEL
    mov ecx, KERNEL_NUM_PAGES
    mov ebx, KERNEL_PHYS_START
    mov edx, 16                     ; we skip the first 16 entries
kernel_map_loop:
    mov eax, ebx                    ; physical address
    or eax, 0x03                    ; PRESENT + RW
    mov [edi + edx*4], eax          ; store PTE
    add ebx, 0x1000                 ; next physical page
    inc edx
    dec ecx
    jnz kernel_map_loop

    ; load CR3 with the page directory address
    mov eax, PAGE_DIR_ADDR
    mov cr3, eax

    ; enable paging by setting PG bit in CR0
    mov eax, cr0
    or eax, 0x80000000 ; set PG bit (bit 31)
    mov cr0, eax

    ret

; ----------------------------------------------------------------
; should look like this
;
;(remote) gef➤  info registers cr3
;cr3            0x20000             [ PDBR=32 PCID=0 ]
;(remote) gef➤  x/10x 0x20000
;0x20000:        0x00030023      0x00000000      0x00000000      0x00000000
;0x20010:        0x00000000      0x00000000      0x00000000      0x00000000
;0x20020:        0x00000000      0x00000000
;(remote) gef➤  x/x 0x20000 + (768 * 4)
;0x20c00:        0x00040003
;(remote) gef➤  x/x 0x40000 + ((0xC0010000 >> 12) & 0x3FF) * 4
;0x40040:        0x00001003
;(remote) gef➤  x/10x 0x1000
;0x1000 <_start>:                0x000007e8      0x0099e800      0xfeeb0000      0x020000bf
;0x1010 <setup_paging+4>:        0x3000b900      0xc0310000      0x00b8aaf3      0x83000300
;0x1020 <setup_paging+20>:       0x00a303c8      0xbf000200
;(remote) gef➤  x/10x 0xC0010000
;0xc0010000:                     0x000007e8      0x0099e800      0xfeeb0000      0x020000bf
;0xc0010010:                     0x3000b900      0xc0310000      0x00b8aaf3      0x83000300
;0xc0010020:                     0x00a303c8      0xbf000200
