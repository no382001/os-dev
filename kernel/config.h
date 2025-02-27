#pragma once

/*
this should be the end of the loaded kernel binary
used in:
    initialise_paging/0
        to identity map the whole kernel
    kmalloc_int/3
        this is the heap before page is set up, consequently this is where the
page tables are
*/
#define PLACEMENT_ADDRESS 0x100000

/*
    heap things
*/
#define KHEAP_START 0xC0100000
#define KHEAP_INITIAL_SIZE 0x100000
#define HEAP_INDEX_SIZE 0x20000
#define HEAP_MAGIC 0x123890AB
#define HEAP_MIN_SIZE 0x70000

// this is set up in kernel entry
#define STACK_BOTTOM 0x60000
#define STACK_SIZE 0x40000 //; 256kb
#define STACK_TOP (STACK_BOTTOM + STACK_SIZE)

// the available pagable RAM
#define END_OF_MEMORY 0x4000000 // 4M

/*
+----------------------+  0x00000
| ...                  |
+----------------------+  0x07C00
| bootloader           |
+----------------------+  0x07E00 (+512b)
| ...                  |
+----------------------+  0x10000
| kernel               |
|                      |
|                      |
+----------------------+  0x60000 (+320kb)
| stack                |
+----------------------+  0xa0000 (+256kb)
| ...                  |
+----------------------+  0x150000 (hard limit 1.25mb,
| initial heap         |            i might not even load that much into ram)
+----------------------+  0x158000 (this how much we id-map, why? to not crash)
| ...                  |
+----------------------+  0xc0040000
| qemu magic sector    |
+----------------------+  0xc0050000
| ...                  |
+----------------------+  0xc0100000
| kernel heap          |
+----------------------+  0xc0200000+ (dynamic)
*/