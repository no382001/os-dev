#include "bits.h"

#define assertm(c, ...)                                                        \
  if (!(c)) {                                                                  \
    serial_printf(__FILE__, __LINE__, __VA_ARGS__);                            \
  }
#define assert(c) assertm(c, " assert failed!")

#include "apps/forth.h"

#define fvm_assert(expr, res)                                                  \
  {                                                                            \
    fvm_repl(&vm, expr);                                                       \
    bool b = !strcmp(res, fvm_test_buffer);                                    \
    assert(!strcmp(res, fvm_test_buffer));                                     \
    if (!b) {                                                                  \
      serial_debug(" expected `%s` got `%s`", res, fvm_test_buffer);           \
    }                                                                          \
  }

void halt() {
  while (1) {
    asm volatile("hlt");
  }
}

char fvm_test_buffer[128] = {0};

void fvm_print_into_buffer(const char *format, ...) {
  va_list args;
  va_start(args, format);
  char s[24] = {0};
  vsprintf(s, 24, format, args);
  strcat(fvm_test_buffer, s);
  strcat(fvm_test_buffer, " ");
  va_end(args);
}

/* */
int fvm_test() {
  serial_puts("starting fvm tests...\n");
  forth_vm_t vm;
  fvm_init(&vm);
  vm.io.stdout = fvm_print_into_buffer;

  /* // these work in adbe7fff7b46ba0bb591e98786b90eaa64e4d0a4
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
  */
  memset((uint8_t *)fvm_test_buffer, 0, 128);
  fvm_assert("10 begin dup . 1 - dup 0 = until drop", "10 9 8 7 6 5 4 3 2 1 ");

  memset((uint8_t *)fvm_test_buffer, 0, 128);
  fvm_assert("0 5 do i . loop", "0 1 2 3 4 ");
  /*
   */

  return 1;
}
/**/

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