#include "page.h"
#include "drivers/serial.h"

#undef serial_debug
#define serial_debug(...)

unsigned int is_paging_enabled() {
  unsigned int cr0;
  asm volatile("mov %%cr0, %0" : "=r"(cr0));
  return (cr0 & 0x80000000);
}

uint8_t page_bitmap[TOTAL_PAGES / 8]; // 1 bit per page

void mark_page_used(uint32_t page_number) {
  page_bitmap[page_number / 8] |= (1 << (page_number % 8));
}

void mark_page_free(uint32_t page_number) {
  page_bitmap[page_number / 8] &= ~(1 << (page_number % 8));
}

uint32_t find_free_page() {
  for (uint32_t i = 0; i < TOTAL_PAGES; i++) {
    if (!(page_bitmap[i / 8] & (1 << (i % 8)))) {
      return i;
    }
  }
  return (uint32_t)-1;
}

void *allocate_page() {
  uint32_t page_number = find_free_page();
  if (page_number == (uint32_t)-1) {
    serial_debug("\tallocate_page:\t no free pages!");
    return NULL;
  } else {
    serial_debug("\tallocate_page:\t got page nr. %d", page_number);
  }

  mark_page_used(page_number);
  return (void *)(KERNEL_HEAP_START + (page_number * PAGE_SIZE));
}

void free_page(void *addr) {
  uint32_t page_number = (uint32_t)addr / PAGE_SIZE;
  mark_page_free(page_number);
}
