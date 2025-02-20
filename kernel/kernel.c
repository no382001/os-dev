
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

void kernel_main(void) {
  kernel_clear_screen();

  isr_install();
  irq_install();
  init_heap();

  serial_init();
  serial_debug("kernel start!");

  selftest();

  /** /
  static forth_vm_t vm = {0};
  fvm_init(&vm);
  fvm = &vm;
  /**/

  /*
   */
  fat_bpb_t bpb;
  fat16_read_bpb(&bpb);

  fs_node_t *root = fs_build_tree(&bpb, 0, NULL, 1);
  fs_print_tree(root, 0);

  fs_node_t *f = fs_find_file(root, "FILE.TXT");

  if (f) {
    char buffer[128] = {0};
    fs_read_file(&bpb, f, (uint8_t *)&buffer);
    hexdump((char *)&buffer, 40, 8);
  }
}
