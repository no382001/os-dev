#include "bits.h"

#undef serial_debug
#define serial_debug(...)

void memcpy(char *dest, const char *source, int no_bytes) {
  int i;
  for (i = 0; i < no_bytes; i++) {
    *(dest + i) = *(source + i);
  }
}

void memset(uint8_t *dest, uint8_t val, uint32_t len) {
  uint8_t *temp = (uint8_t *)dest;
  for (; len != 0; len--)
    *temp++ = val;
}

#define HEAP_START 0xC0100000
static mem_block_t *heap_start = NULL;

void init_heap() {
  kernel_printf("- setting up heap\n");
  heap_start = (mem_block_t *)allocate_page();
  heap_start->size = PAGE_SIZE - sizeof(mem_block_t);
  heap_start->used = false;
  heap_start->next = NULL;
}

mem_block_t *find_free_block(uint32_t size) {
  mem_block_t *current = heap_start;
  while (current) {
    if (!current->used && current->size >= size) {
      return current;
    }
    current = current->next;
  }
  return NULL;
}

void *malloc(size_t size) {
  if (size == 0)
    return NULL;

  serial_debug("\tmalloc:\t requested size: %d", size);

  mem_block_t *block = find_free_block(size);
  if (!block) {
    serial_debug("\tmalloc:\t no free block, allocating new page.");

    block = (mem_block_t *)allocate_page();
    if (!block) {
      serial_debug("\tmalloc:\t allocate_page() failed! No memory left. %x",
                   &block);
      return NULL;
    }

    block->size = PAGE_SIZE - sizeof(mem_block_t);
    block->used = false;
    block->next = heap_start;
    heap_start = block;
  }

  serial_debug("\tmalloc:\t block allocated at %x", &block);

  block->used = true;

  if (block->size > size + sizeof(mem_block_t)) {
    mem_block_t *new_block =
        (mem_block_t *)((uint8_t *)block + sizeof(mem_block_t) + size);
    new_block->size = block->size - size - sizeof(mem_block_t);
    new_block->used = false;
    new_block->next = block->next;
    block->next = new_block;
    block->size = size;
  }

  return (void *)((uint8_t *)block + sizeof(mem_block_t));
}

void free(void *ptr) {
  if (!ptr)
    return;

  mem_block_t *block = (mem_block_t *)((uint8_t *)ptr - sizeof(mem_block_t));
  block->used = false;
  serial_debug("\tfree:\t freed block at %x", block);

  if (block->next && !block->next->used) {
    serial_debug("\tfree:\t merging with next free block.");
    block->size += sizeof(mem_block_t) + block->next->size;
    block->next = block->next->next;
  }

  mem_block_t *prev = heap_start;
  while (prev && prev->next != block) {
    prev = prev->next;
  }

  if (prev && !prev->used) {
    serial_debug("\tfree:\t merging with previous free block.");
    prev->size += sizeof(mem_block_t) + block->size;
    prev->next = block->next;
  }
}

void print_heap() {
  serial_debug("\theap dump:");
  mem_block_t *current = heap_start;
  while (current) {
    serial_debug("\tblock at %x - size: %d, used: %d", current, current->size,
                 current->used);
    current = current->next;
  }
}

int heap_block_count() {
  int i = 0;
  mem_block_t *current = heap_start;
  while (current) {
    i++;
    current = current->next;
  }
  return i;
}