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

typedef uint32_t page_entry_t;

void set_page_entry(uint32_t virtual_addr, uint32_t physical_addr) {
  uint32_t page_dir_index = (virtual_addr >> 22) & 0x3FF;
  uint32_t page_table_index = (virtual_addr >> 12) & 0x3FF;

  page_entry_t *page_dir = (page_entry_t *)PAGE_DIR_ADDR;
  page_entry_t *page_table = (page_entry_t *)PAGE_TABLE_KERNEL;

  if (!(page_dir[page_dir_index] & PAGE_PRESENT)) {
    return;
  }

  page_table[page_table_index] = physical_addr | PAGE_PRESENT | PAGE_RW;

  asm volatile("movl %%cr3, %%eax\n\t" // move CR3 into EAX
               "movl %%eax, %%cr3\n\t" // write back to CR3 to flush TLB
               ::
                   : "eax" // clobber list: EAX is modified
  );
}

void *allocate_page() {
  uint32_t page_number = find_free_page();
  if (page_number == (uint32_t)-1) {
    serial_debug("\tallocate_page:\t no free pages!");
    return NULL;
  }

  mark_page_used(page_number);

  uint32_t physical_addr = page_number * PAGE_SIZE;
  uint32_t virtual_addr = KERNEL_HEAP_START + physical_addr;

  set_page_entry(virtual_addr, physical_addr);

  return (void *)virtual_addr;
}

void free_page(void *addr) {
  uint32_t page_number = (uint32_t)addr / PAGE_SIZE;

  mark_page_free(page_number);

  set_page_entry((uint32_t)addr, 0);
}

void debug_page_directory() {
  uint32_t *page_dir = (uint32_t *)PAGE_DIR_ADDR;
  (void)page_dir;
  for (int i = 0; i < 10; i++) {
    serial_debug(" PDE[%d] = 0x%x", i, page_dir[i]);
  }
}

void debug_page_table(uint32_t virtual_addr) {
  uint32_t page_dir_index = (virtual_addr >> 22) & 0x3FF;
  uint32_t page_table_index = (virtual_addr >> 12) & 0x3FF;

  uint32_t *page_dir = (uint32_t *)PAGE_DIR_ADDR;
  uint32_t *page_table = (uint32_t *)PAGE_TABLE_KERNEL;
  (void)page_dir;
  (void)page_table;
  (void)page_table_index;

  if (!(page_dir[page_dir_index] & 1)) {
    serial_debug("page directory entry is not present!");
    return;
  }

  serial_debug(" PTE[%d] for virtual Addr 0x%x = 0x%x", page_table_index,
               virtual_addr, page_table[page_table_index]);
}