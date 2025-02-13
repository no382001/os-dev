#include "bits.h"

void selftest(void);
void kernel_main(void) {
  kernel_clear_screen();

  isr_install();
  irq_install();

  serial_init();
  init_heap();

  selftest();
}
