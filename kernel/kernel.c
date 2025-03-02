
#include "bits.h"

void selftest(void);
void fvm_test(void);

forth_vm_t *g_fvm = 0;

void user_input(char *input) {
  if (!g_fvm) {
    return;
  }
  fvm_repl(g_fvm, input);
  if (g_fvm->err) {
    g_fvm->err = 0;
  } else {
    kernel_printf(" ok\n");
  }
}

extern uint8_t *vga_bb;
semaphore_t task_semaphore = {0};

void kernel_main(void) {
  kernel_clear_screen();

  isr_install();
  irq_install();

  initialise_paging();

  selftest();

  set_vga_mode12();
  vga_bb = (uint8_t *)kmalloc(VGA_BUFFER_SIZE * 4);

  vga12h_gradient_demo();
}
