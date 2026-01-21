#pragma once

#include "libc/types.h"

/*
 * GRUB loads kernel at 1MB (0x100000)
 * stack is in .bss (defined in entry.asm)
 */

// kernel is loaded here by GRUB
#define KERNEL_LOAD_ADDR 0x100000

// placement allocator starts after kernel
// _kernel_end is defined in linker script
extern uint32_t _kernel_end;
#define PLACEMENT_ADDRESS ((uint32_t) & _kernel_end)

/*
 * heap configuration
 * we'll put heap after the placement allocator area
 */
#define KHEAP_START 0x400000        // 4MB - safe distance from kernel
#define KHEAP_INITIAL_SIZE 0x100000 // 1MB initial
#define HEAP_INDEX_SIZE 0x20000
#define HEAP_MAGIC 0x123890AB
#define HEAP_MIN_SIZE 0x70000

// stack is now in .bss, defined in entry.asm
// these are just for reference/stack guard checking
#define STACK_SIZE 0x40000 // 256KB

/*
 * framebuffer address comes from multiboot info at runtime
 * could be anywhere - often above 0xFD000000 on real hardware
 */

/*
 * memory layout with GRUB multiboot:
 *
 * +----------------------+  0x00000
 * | Real mode IVT, BDA   |
 * +----------------------+  0x00500
 * | Free                 |
 * +----------------------+  0x07C00
 * | (unused, was bootldr)|
 * +----------------------+  0x80000
 * | EBDA (varies)        |
 * +----------------------+  0xA0000
 * | VGA memory           |
 * +----------------------+  0xC0000
 * | ROM area             |
 * +----------------------+  0x100000 (1MB)
 * | Kernel .text         |
 * | Kernel .rodata       |
 * | Kernel .data         |
 * | Kernel .bss (stack)  |
 * +----------------------+  _kernel_end
 * | Placement allocator  |
 * +----------------------+  (dynamic)
 * | ...                  |
 * +----------------------+  0x400000 (4MB)
 * | Kernel heap          |
 * +----------------------+  0x500000+ (grows)
 * | ...                  |
 * +----------------------+  (from multiboot mmap)
 * | Usable RAM end       |
 * +----------------------+
 *
 * framebuffer is mapped by BIOS/UEFI, address from multiboot info
 * often at 0xFD000000+ or similar high address
 */

uint32_t get_memory_size(void);
void init_memory_from_multiboot(void *mbi);