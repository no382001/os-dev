#include "screen.h"

void main() {
  kernel_clear_screen();
  char s[] = "hello kernel!\n .rodata is misaligned -> ";
  // .rodata is mapped incorrectly so dont use ptrs
  char* s1 = "hello .rodata!";
  kernel_puts(s);
  kernel_puth(s1);
}
