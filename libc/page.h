#pragma once
#include "bits.h"
#include "cpu/isr.h"

#define PAGE_SIZE 0x1000

typedef struct page {
  uint32_t present : 1;
  uint32_t rw : 1;
  uint32_t user : 1;
  uint32_t accessed : 1; // has the page been accessed since last refresh?
  uint32_t dirty : 1;    // has the page been written to since last refresh?
  uint32_t unused : 7;
  uint32_t frame : 20; // frame address (shifted right 12 bits)
} page_t;

typedef struct page_table {
  page_t pages[1024];
} page_table_t;

typedef struct page_directory {
  page_table_t *tables[1024];
  uint32_t tablesPhysical[1024]; // give this to cr3 as ref
  uint32_t physicalAddr;
} page_directory_t;

void initialise_paging();
void switch_page_directory(page_directory_t *new);
page_t *get_page(uint32_t address, int make, page_directory_t *dir);

void page_fault(registers_t *regs);
unsigned int is_paging_enabled();

void alloc_frame(page_t *page, int is_kernel, int is_writeable);
void free_frame(page_t *page);

uint32_t virtual2phys(page_directory_t *dir, void *virtual_addr);
// return the phys address based on the kernel pagedir
uint32_t virt2phys(void *virtual_addr);