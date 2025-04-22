
#include "bits.h"

void selftest(void);
void fvm_test(void);

semaphore_t task_semaphore = {0};

void kernel_main(void) {
  kernel_clear_screen();

  isr_install();
  irq_install();

  initialise_paging();

  selftest();

  set_vga_mode12();
  init_vga12h();

  vga_demo_terminal();
  // vga12h_gradient_demo();
}
