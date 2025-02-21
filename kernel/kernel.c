
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

extern uint32_t tick;
void sleep(uint32_t milliseconds) {
  uint32_t target_ticks =
      tick + (milliseconds * 60) / 1000; // (assuming 60Hz PIT)
  while (tick < target_ticks) {
    asm volatile("hlt"); // Halt CPU until the next interrupt
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

  fat_bpb_t bpb;
  fat16_read_bpb(&bpb);

  fs_node_t *root = fs_build_tree(&bpb, 0, NULL, 1);
  fs_print_tree(root, 0);

  fs_node_t *f = fs_find_file(root, "FILE.TXT");

  if (f) {
    char *buffer = malloc(f->size);
    fs_read_file(&bpb, f, (uint8_t *)&buffer);
    hexdump((char *)&buffer, 40, 8);
  }

  /**/
  set_vga_mode12();

  int rect_width = 200;
  int rect_height = 100;

  int center_x = (VGA_WIDTH / 2) - (rect_width / 2);
  int center_y = (VGA_HEIGHT / 2) - (rect_height / 2);

  int i = 0;
  while (1) {
    draw_scrolling_gradient(i++);
    for (int j = 0; j < rect_width; j++) {
      for (int k = 0; k < rect_height; k++) {
        vga_put_pixel(center_x + j, center_y + k, 0);
      }
    }
    sleep(16); // 60fps?
  }
}
