
#include "bits.h"

void selftest(void);
void fvm_test(void);

forth_vm_t *fvm = 0;

void user_input(char *input) {
  if (!fvm) {
    return;
  }

  fvm_repl(fvm, input);
}

void kernel_main(void) {
  kernel_clear_screen();

  isr_install();
  irq_install();
  init_heap();

  serial_init();
  serial_debug("kernel start!");

  selftest();

  /**/
  static forth_vm_t vm = {0};
  fvm_init(&vm);
  fvm = &vm;
  /**/

  /*
  fat_bpb_t bpb;
  fat16_read_bpb(&bpb);
  fat16_list_root(&bpb);
  */
}
