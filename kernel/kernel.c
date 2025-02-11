#include "bits.h"

unsigned int is_paging_enabled() {
  unsigned int cr0;
  asm volatile("mov %%cr0, %0" : "=r"(cr0));
  return (cr0 & 0x80000000);
}


void kernel_main(void) {
  kernel_clear_screen();

  isr_install();
  irq_install();

  serial_init();

  kernel_puts("hello kernel!\n");
  serial_debug("hello serial!\n");

  /* Lesson 22: Code to test kmalloc, the rest is unchanged */
  size_t phys_addr;
  size_t page = kernel_malloc(1000, 1, &phys_addr);
  char page_str[16] = "";
  hex_to_ascii(page, page_str);
  char phys_str[16] = "";
  hex_to_ascii(phys_addr, phys_str);
  kernel_puts("Page: ");
  kernel_puts(page_str);
  kernel_puts(", physical address: ");
  kernel_puts(phys_str);
  char *s = (char *)kernel_malloc(11, 1, 0);
  memcpy("0123456789", s, 11);
  kernel_puts("\n");
  kernel_puts(s);

  if (is_paging_enabled()){
    kernel_puts("\n");
    kernel_puts("PAGING ENABLED!");
  }
}
