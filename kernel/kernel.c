#include "bits.h"

void selftest(void);

void kernel_main(void) {
  kernel_clear_screen();

  isr_install();
  irq_install();
  init_heap();

  serial_init();
  serial_debug(" start!");

  selftest();

  fat_bpb_t bpb;
  fat16_read_bpb(&bpb);
  fat16_list_root(&bpb);

  hexdump("012345678910", 13, 4);
  hexdump("Hello World!", 13, 8);

  forth_vm_t ctx;
  fvm_init(&ctx);
  fvm_repl(&ctx,"1 2 + .");

  serial_debug(" at the end...");
}
