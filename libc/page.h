#pragma once
#include "libc/types.h"

#define PAGE_SIZE 0x1000 // 4KB per page
#define TOTAL_PAGES 1024 // 4MB total memory (1024 * 4KB)
#define KERNEL_HEAP_START 0x0050000

#define PAGE_DIR_ADDR 0x0020000
#define PAGE_TABLE_KERNEL 0x0040000

#define PAGE_PRESENT 0x1
#define PAGE_RW 0x2
#define PAGE_USER 0x4

void *allocate_page();
unsigned int is_paging_enabled();
void debug_page_directory();