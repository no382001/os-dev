
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

uint32_t get_stack_usage() {
  uint32_t esp;
  asm volatile("mov %%esp, %0" : "=r"(esp));

  if (esp < (uint32_t)STACK_BOTTOM || esp > (uint32_t)STACK_TOP) {
    return 0;
  }

  return (uint32_t)STACK_TOP - esp;
}

extern uint8_t *vga_bb;
void kernel_main(void) {
  kernel_clear_screen();

  isr_install();
  irq_install();

  initialise_paging();
  kernel_printf("%db\n", get_stack_usage());

  g_fvm = (forth_vm_t *)kmalloc(sizeof(forth_vm_t));
  fvm_init(g_fvm);

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
