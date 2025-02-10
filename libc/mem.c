#include "mem.h"

void memcpy(char *source, char *dest, int no_bytes) {
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

/* this is baked in! it should be calc'd link time
   kernel starts at 0x1000, currently */
size_t free_mem_addr = 0x10000;
size_t kernel_malloc(size_t size, int align, size_t *phys_addr) {
  /* pages are aligned to 4K, or 0x1000
     however the pages are supposedly misaligned and should be masked w/
     0x00000FFF on the condition check but that seems to not work so...
     */
  if (align == 1 && (free_mem_addr & 0xFFFFF000)) {
    free_mem_addr &= 0xFFFFF000;
    free_mem_addr += 0x1000;
  }
  if (phys_addr)
    *phys_addr = free_mem_addr;

  size_t ret = free_mem_addr;
  free_mem_addr += size;
  return ret;
}
