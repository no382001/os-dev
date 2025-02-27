
#include "bits.h"

void selftest(void);
void fvm_test(void);

void user_input(char *input) { (void)input; }

extern uint8_t *vga_bb;
void kernel_main(void) {
  kernel_clear_screen();

  isr_install();
  irq_install();
  serial_debug("start!");
  kernel_printf("start!!\n");

  uint32_t a = kmalloc(8);

  initialise_paging();

  uint32_t b = kmalloc(8);
  uint32_t c = kmalloc(8);

  serial_printff("a: %x", a);
  serial_printff("b: %x", b);
  serial_printff("c: %x", c);

  kfree((void *)c);
  kfree((void *)b);
  uint32_t d = kmalloc(12);
  serial_printff("d: %x", d);
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
