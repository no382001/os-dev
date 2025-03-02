
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
  uint32_t esp, ebp;
  asm volatile("mov %%esp, %0" : "=r"(esp));
  asm volatile("mov %%esp, %0" : "=r"(ebp));

  if (esp < (uint32_t)STACK_BOTTOM || esp > (uint32_t)STACK_TOP) {
    return 0;
  }

  return (uint32_t)ebp - esp;
}

extern uint8_t *vga_bb;

semaphore_t task_semaphore = {0};
extern int current_task_idx;
void taks_first() {
  while (1) {
    // semaphore_wait(&task_semaphore);

    uint32_t esp, ebp;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    asm volatile("mov %%esp, %0" : "=r"(ebp));

    kernel_printf("esp %x, ebp %x in task %d\n", esp, ebp, current_task_idx);
    // semaphore_signal(&task_semaphore);
  }
}

void kernel_main(void) {
  kernel_clear_screen();

  isr_install();
  irq_install();

  initialise_paging();
  g_fvm = (forth_vm_t *)kmalloc(sizeof(forth_vm_t));
  fvm_init(g_fvm);

  selftest();

  semaphore_init(&task_semaphore, 1);
  // init_tasking();
  asm volatile("cli");
  create_task(0, taks_first, (void *)0x80000);
  create_task(1, taks_first, (void *)0x70000);
  create_task(2, taks_first, (void *)0x90000);
  create_task(3, taks_first, (void *)0x85000);
  asm volatile("sti");
  while (1) {
    asm volatile("hlt");
  }
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
