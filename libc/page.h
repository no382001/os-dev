#pragma once
#include "libc/types.h"

#define PAGE_SIZE 0x1000 // 4KB per page
#define TOTAL_PAGES 1024 // 4MB total memory (1024 * 4KB)
#define KERNEL_HEAP_START 0x0050000

void *allocate_page();
unsigned int is_paging_enabled();