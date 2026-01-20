
#include "bits.h"

void selftest(void);
void fvm_test(void);

semaphore_t task_semaphore = {0};

// task test helpers
static volatile int task_test_counter = 0;
static volatile int task1_ran = 0;
static volatile int task2_ran = 0;
static volatile int task3_ran = 0;
static volatile uint32_t task_data_value = 0;

static void sched_test_fn(void *data) {
  (void)data;
  task1_ran = 1;
  task_test_counter++;
  kernel_printf("  task1 executed\n");
}

static void sched_test_fn2(void *data) {
  (void)data;
  task2_ran = 1;
  task_test_counter++;
  kernel_printf("  task2 executed\n");
  serial_debug("task2 executed successfully");
}

static void sched_test_fn3(void *data) {
  uint32_t *value = (uint32_t *)data;
  kernel_printf("  task3 started, data=%x, value=%x\n", data,
                value ? *value : 0);
  if (value && *value == 0xCAFEBABE) {
    task_data_value = *value;
    task3_ran = 1;
    task_test_counter++;
    kernel_printf("  task3 executed with data=%x\n", *value);
  } else {
    kernel_printf("  task3 data check failed!\n");
  }
}

static void task_destructor_with_free(void *df_data) {
  if (df_data) {
    kfree(df_data);
  }
}

void selftest() {
  kernel_printf("=== running kernel self-tests ===\n");

  // test 1: basic heap allocation
  void *p = kmalloc(26);
  assert(p && "malloc does not work");
  kfree(p);
  void *p1 = kmalloc(26);
  assert(p == p1 && "not the same address");
  kernel_printf("- kmalloc/kfree looks good\n");
  kfree(p1);

  // test 2: multiple allocations and free ordering
  void *a1 = kmalloc(128);
  void *a2 = kmalloc(256);
  void *a3 = kmalloc(64);
  assert(a1 && a2 && a3 && "multiple allocs failed");
  kfree(a2); // free middle block
  kfree(a1);
  kfree(a3);
  kernel_printf("- multiple alloc/free works\n");

  // test 3: aligned allocation
  void *aligned = kmalloc_a(512);
  assert(aligned && "aligned alloc failed");
  assert(((uint32_t)aligned & 0xFFF) == 0 && "not page aligned");
  // note: aligned allocations cannot be freed with current allocator
  kernel_printf("- aligned allocation works\n");

  // test 4: string operations
  char str1[32] = "hello";
  char str2[32] = "world";
  assert(strlen(str1) == 5 && "strlen failed");
  assert(strcmp(str1, "hello") == 0 && "strcmp failed");
  assert(strcmp(str1, str2) != 0 && "strcmp should differ");
  strcpy(str2, str1);
  assert(strcmp(str1, str2) == 0 && "strcpy failed");
  strcat(str2, "!");
  assert(strlen(str2) == 6 && "strcat failed");
  kernel_printf("- string operations work\n");

  // test 5: memory operations
  uint8_t buf1[64];
  uint8_t buf2[64];
  memset(buf1, 0xAA, 64);
  memset(buf2, 0xBB, 64);
  assert(buf1[0] == 0xAA && buf1[63] == 0xAA && "memset failed");
  memcpy(buf2, buf1, 64);
  assert(memcmp(buf1, buf2, 64) == 0 && "memcpy failed");
  memset(buf2, 0xCC, 32);
  assert(memcmp(buf1, buf2, 64) != 0 && "memcmp should differ");
  kernel_printf("- memory operations work\n");

  // test 6: sprintf/snprintf
  char numstr[32];
  int written = snprintf(numstr, sizeof(numstr), "num=%d hex=%x", 42, 0xDEAD);
  assert(written > 0 && "snprintf failed");
  assert((int)strlen(numstr) == written && "snprintf length mismatch");
  kernel_printf("- sprintf/snprintf works\n");

  // test 7: timer ticks
  int t1 = get_tick();
  sleep(100);
  int t2 = get_tick();
  assert(t1 != t2 && "ticks are the same");
  assert(t2 > t1 && "ticks not incrementing");
  kernel_printf("- timer ticks work\n");

  // test 8: semaphore operations
  semaphore_t test_sem;
  semaphore_init(&test_sem, 1);
  assert(semaphore_count(&test_sem) == 1 && "semaphore init failed");
  semaphore_wait(&test_sem);
  assert(semaphore_count(&test_sem) == 0 && "semaphore wait failed");
  semaphore_signal(&test_sem);
  assert(semaphore_count(&test_sem) == 1 && "semaphore signal failed");
  kernel_printf("- semaphore operations work\n");

  // test 9: paging functions
  page_t *test_page = get_page(0xD0000000, 0, kernel_directory);
  assert(test_page == 0 && "unmapped page should be null");
  test_page = get_page(0xD0000000, 1, kernel_directory);
  assert(test_page != 0 && "get_page with make=1 failed");
  kernel_printf("- paging functions work\n");

  // test 10: basic task scheduling
  kernel_printf("- testing task scheduling...\n");
  task_test_counter = 0;
  task1_ran = 0;

  uint32_t stack_size = 1024 * 10;
  void *task_stack1 = kmalloc_a(stack_size);
  assert(task_stack1 && "task stack allocation failed");

  create_task("test_task1", &sched_test_fn, 0, &task_destructor_with_free,
              task_stack1, task_stack1, stack_size);

  // give scheduler time to start the task
  sleep(50);

  // wait for task to execute - sleep allows timer interrupts to trigger
  // scheduler
  int timeout = 50;
  while (task1_ran == 0 && timeout-- > 0) {
    sleep(20);
  }
  assert(task1_ran == 1 && "task1 did not execute");
  assert(task_test_counter >= 1 && "task counter not incremented");
  kernel_printf("  basic task scheduling works\n");

  // give task1 time to fully cleanup and free its slot
  sleep(100);

  // test 11: multiple concurrent tasks
  kernel_printf("- testing multiple tasks...\n");
  task2_ran = 0;
  task3_ran = 0;

  void *task_stack2 = kmalloc_a(stack_size);
  void *task_stack3 = kmalloc_a(stack_size);
  assert(task_stack2 && task_stack3 && "multiple task stacks failed");

  uint32_t *test_data = kmalloc(sizeof(uint32_t));
  *test_data = 0xCAFEBABE;

  serial_debug("creating test_task2...");
  create_task("test_task2", &sched_test_fn2, 0, &task_destructor_with_free,
              task_stack2, task_stack2, stack_size);
  serial_debug("creating test_task3...");
  create_task("test_task3", &sched_test_fn3, test_data, 0, 0, task_stack3,
              stack_size);
  serial_debug("both tasks created");

  // give scheduler time to start the tasks
  sleep(150);

  // wait for both tasks - sleep allows timer interrupts to trigger scheduler
  timeout = 250;
  while ((task2_ran == 0 || task3_ran == 0) && timeout-- > 0) {
    sleep(20);
  }

  assert(task2_ran == 1 && "task2 did not execute");
  assert(task3_ran == 1 && "task3 did not execute");
  assert(task_data_value == 0xCAFEBABE && "task data not passed correctly");
  assert(task_test_counter >= 3 && "not all tasks ran");

  kfree(test_data);
  kernel_printf("  multiple tasks work\n");

  // give tasks time to cleanup
  sleep(50);
  kernel_printf("- task scheduling tests passed\n");

  // test 11: setjmp/longjmp
  jmp_buf env = {0};
  if (setjmp(env) == 0) {
    longjmp(env, 42);
    kernel_printf("jump failed!\n");
  } else {
    kernel_printf("- setjmp/longjmp works\n");
  }

  // test 12: large allocation
  void *large = kmalloc(8192);
  assert(large && "large allocation failed");
  memset(large, 0x55, 8192);
  assert(((uint8_t *)large)[0] == 0x55 && "large alloc write failed");
  assert(((uint8_t *)large)[8191] == 0x55 && "large alloc boundary failed");
  kfree(large);
  kernel_printf("- large allocations work\n");

  kernel_printf("=== all self-tests passed! ===\n");
  serial_debug("selftest finished!");
}

vfs *init_vfs();
void vfs_print_current_tree(vfs *fs);

void kernel_main(void) {
  // be very careful, sometimes un-inited modules work even in kvm, for some
  // time, then they 3F
  kernel_clear_screen();

  isr_install();
  irq_install();

  initialise_paging();

  serial_debug("paging done...");

  kernel_printf("- initializing pci...\n");
  pci_init();
  serial_debug("pci done...");

  // set_vga_mode12();
  // init_vga12h();
  // vga_demo_terminal();
  // vga12h_gradient_demo();

  xinit();
  init_tasking();
  selftest();

  vfs *unified_vfs = init_vfs();

  vfs_print_current_tree(unified_vfs);

  while (1) {
    ;
  }
}
