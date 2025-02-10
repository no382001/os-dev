#include "bits.h"

void main(void) {
  kernel_clear_screen();

  isr_install();
  irq_install();
  serial_init();

  kernel_puts("hello kernel!\n");
  serial_debug("hello serial!\n");
}
