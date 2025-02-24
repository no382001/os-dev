
#include "bits.h"

void selftest(void);
void fvm_test(void);

forth_vm_t *fvm = 0;

void user_input(char *input) {
  if (!fvm) {
    return;
  }
  fvm_repl(fvm, input);
  if (fvm->err) {
    fvm->err = 0;
  } else {
    kernel_printf(" ok\n");
  }
}

extern uint8_t *vga_bb;
void kernel_main(void) {
  kernel_clear_screen();

  isr_install();
  irq_install();
  serial_debug("start!");
  // kernel_printf("%db\n", get_stack_usage());
  initialise_paging();
  serial_debug("end!");

  /*
  init_heap();

  serial_init();
  serial_debug("kernel start!");

  selftest();

  uint8_t backbuffer[VGA_BUFFER_SIZE * 4] = {0};
  vga_bb = (uint8_t *)&backbuffer;

  serial_debug("stack usage: ~%dkb", (int)get_stack_usage() / 1024);

  set_vga_mode12();
  vga12h_gradient_demo();
   */
}
