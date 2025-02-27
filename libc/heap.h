#pragma once
#include "kernel/config.h"
#include "libc/ordered_array.h"

typedef struct {
  uint32_t magic;
  int8_t is_hole;
  uint32_t size;
} header_t;

typedef struct {
  uint32_t magic;
  header_t *header;
} footer_t;

typedef struct {
  ordered_array_t index;
  uint32_t start_address;
  uint32_t end_address;
  uint32_t max_address;
  int8_t supervisor;
  int8_t readonly;
} heap_t;

heap_t *create_heap(uint32_t start, uint32_t end, uint32_t max,
                    int8_t supervisor, int8_t readonly);
void *alloc(uint32_t size, int8_t page_align, heap_t *heap);
void free(void *p, heap_t *heap);

uint32_t kmalloc_int(uint32_t sz, int align, uint32_t *phys);
uint32_t kmalloc_a(uint32_t sz);
uint32_t kmalloc_p(uint32_t sz, uint32_t *phys);
uint32_t kmalloc_ap(uint32_t sz, uint32_t *phys);
uint32_t kmalloc(uint32_t sz);

void kfree(void *p);
