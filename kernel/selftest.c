#include "bits.h"

#define assertm(c, ...)                                                        \
  if (!(c)) {                                                                  \
    serial_printf(__FILE__, __LINE__, __VA_ARGS__);                            \
  }
#define assert(c) assertm(c, " assert failed!")

#include "apps/forth.h"

#define fvm_assert(expr, res) fvm_repl(&vm, expr);

void halt() {
  while (1) {
    asm volatile("hlt");
  }
}

int fvm_test() {
  serial_puts("starting fvm tests...\n");
  forth_vm_t vm;
  fvm_init(&vm);

  fvm_assert("1 2 + .", "3");
  fvm_assert("5 3 - .", "2");
  fvm_assert("4 2 * .", "8");
  fvm_assert("10 2 / .", "5");

  fvm_assert("10 5 > .", "1");
  fvm_assert("10 5 < .", "0");
  fvm_assert("5 5 = .", "1");
  fvm_assert("5 10 = .", "0");
  fvm_assert("5 10 <> .", "1");
  fvm_assert("5 5 <> .", "0");

  fvm_assert("0 not .", "1");
  fvm_assert("1 not .", "0");

  fvm_assert("7 dup .", "7");
  fvm_assert(".", "7");

  fvm_assert("1 2 swap .", "1");
  fvm_assert(".", "2");

  // fvm_assert("9 drop", ""); // works

  fvm_assert("3 4 over .", "3");
  fvm_assert(".", "4");
  fvm_assert(".", "3");

  fvm_repl(&vm, "42 0 !");
  fvm_assert("0 @ .", "42");

  fvm_assert("5 5 = if 1 . else 0 . then", "1");
  fvm_assert("5 10 = if 1 . else 0 . then", "0");

  fvm_assert("10 begin dup . 1 - dup 0 = until drop", "10 9 8 7 6 5 4 3 2 1");

  /*
  fvm_assert("0 5 do i . loop", "0 1 2 3 4");
  */

  return 1;
}

/*
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

  kernel_printf("- starting fvm tests\n");
  fvm_test();
  kernel_printf("  - done\n");
}
*/