#include "bits.h"
/**/
// have a `failed` variable ready
#define assertm(c, ...)                                                        \
  if (!(c)) {                                                                  \
    serial_printf(__FILE__, __LINE__, __VA_ARGS__);                            \
    failed++;                                                                  \
  }

#define assert_eq(c)                                                           \
  { assertm(c, " assert_eq failed!"); }

#include "apps/forth.h"

#define fvm_assert(expr, res)                                                  \
  {                                                                            \
    fvm_repl(&vm, expr);                                                       \
    fvm_test_buffer[strlen(fvm_test_buffer) - 1] = 0;                          \
    bool b = !strcmp(res, fvm_test_buffer);                                    \
    assert_eq(!strcmp(res, fvm_test_buffer));                                  \
    if (!b) {                                                                  \
      serial_debug(" expected `%s` got `%s`", res, fvm_test_buffer);           \
    }                                                                          \
    memset((uint8_t *)fvm_test_buffer, 0, 128);                                \
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
  strcat(fvm_test_buffer, " "); // delete the last space in the macro idc
  va_end(args);
}
/*
int fvm_test() {
  int failed = 0;

  forth_vm_t vm;
  fvm_init(&vm);
  vm.io.stdout = fvm_print_into_buffer;
  memset((uint8_t *)fvm_test_buffer, 0, 128);

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

  fvm_assert("9 drop", "");

  fvm_assert("3 4 over .", "3");
  fvm_assert(".", "4");
  fvm_assert(".", "3");

  fvm_repl(&vm, "42 0 !");
  fvm_assert("0 @ .", "42");
  fvm_assert("5 5 = if 1 . else 0 . then", "1");
  fvm_assert("5 10 = if 1 . else 0 . then", "0");

  fvm_assert("10 begin dup . 1 - dup 0 = until drop", "10 9 8 7 6 5 4 3 2 1");

  fvm_assert("0 5 do i . loop", "0 1 2 3 4");

  return failed;
}

*/
int core_tests(void) {
  int failed = 0;

  assertm(is_paging_enabled(), "paging is not enabled!");

  char *s = (char *)kmalloc(12);
  assertm(s, "malloc returned nullptr!");
  memcpy(s, "hello heap!", 12);
  assertm(!strcmp(s, "hello heap!"), "heap string doesnt match!");

  kfree(s);

  return failed;
}

#define run_test(fn)                                                           \
  {                                                                            \
    kernel_printf("- starting " #fn "...");                                    \
    int f = fn();                                                              \
    if (f == 0) {                                                              \
      kernel_printf(" ok\n");                                                  \
    } else {                                                                   \
      kernel_printf(" #%d failed\n", f);                                       \
    }                                                                          \
  }

void selftest(void) {
  run_test(core_tests);
  // run_test(fvm_test);
}
