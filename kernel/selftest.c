#include "bits.h"

#define assertm(c, ...)                                                        \
  if (!(c)) {                                                                  \
    serial_printf(__FILE__, __LINE__, __VA_ARGS__);                            \
  }
#define assert(c) assertm(c, "assert failed!")

extern mem_block_t *heap_start;

void selftest(void) {
  kernel_printf("- starting selftest... check serial\n");
  serial_puts("starting selftest...\n");

  assertm(is_paging_enabled(), "paging is not enabled!");

  assert(heap_block_count() == 1);

  char *s = (char *)malloc(12);
  assertm(s, "malloc returned nullptr!");
  memcpy(s, "hello heap!", 12);
  assertm(!strcmp(s, "hello heap!"), "heap string doesnt match!");

  free(s);

  assert(heap_block_count() == 1);
  kernel_printf("  - done\n");
}