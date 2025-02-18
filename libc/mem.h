#pragma once

#include "types.h"

typedef struct mem_block mem_block_t;
struct mem_block {
  uint32_t size;
  bool used;
  mem_block_t *next;
};

void init_heap();
void print_heap();

void memcpy(char *dest, const char *source, int no_bytes);
void memset(uint8_t *dest, uint8_t val, uint32_t len);

void *malloc(size_t size);
void free(void *ptr);

int heap_block_count();