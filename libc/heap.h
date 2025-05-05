#pragma once
#include "kernel/config.h"
#include "libc/ordered_array.h"

typedef struct kernel_heap_bitmap_block kernel_heap_bitmap_block_t;

struct kernel_heap_bitmap_block {
  kernel_heap_bitmap_block_t *next;
  uint32_t block_size;
  uint32_t used_blocks;
  uint32_t allocation_unit_size;
  uint32_t last_free_block;
};

typedef struct {
  kernel_heap_bitmap_block_t *first_block;
} kernel_heap_bitmap_t;

void kernel_heap_bitmap_init(kernel_heap_bitmap_t *heap);
int kernel_heap_bitmap_add_block(kernel_heap_bitmap_t *heap, uintptr_t addr,
                                 uint32_t size, uint32_t block_size);
void *kernel_heap_bitmap_alloc(kernel_heap_bitmap_t *heap, uint32_t size);
void kernel_heap_bitmap_free(kernel_heap_bitmap_t *heap, void *ptr);

uint32_t kmalloc_int(uint32_t sz, int align, uint32_t *phys);
uint32_t kmalloc_a(uint32_t sz);
uint32_t kmalloc_p(uint32_t sz, uint32_t *phys);
uint32_t kmalloc_ap(uint32_t sz, uint32_t *phys);
uint32_t kmalloc(uint32_t sz);
void kfree(void *p);
