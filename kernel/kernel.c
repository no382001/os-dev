
#include "bits.h"

void selftest(void);
void fvm_test(void);

semaphore_t task_semaphore = {0};

static volatile int sem = 0;

static void sched_test_fn(void *data) {
  (void)data;
  // semaphore_signal(&selftest_sem);
  sem = 1;
  kernel_printf("- scheduler looks good\n");
  return;
}

static void sched_test_dfn(void *df_data) {
  kfree(df_data);
  return;
}

void selftest() {
  void *p = kmalloc(26);
  assert(p && "malloc does not work");
  kfree(p);
  void *p1 = kmalloc(26);
  assert(p == p1 && "not the same address");
  kernel_printf("- kmalloc looks good...\n");
  kfree(p1);

  int t1 = get_tick();
  sleep(100);
  int t2 = get_tick();
  assert(t1 != t2 && "ticks are the same");
  kernel_printf("- ticks are good...\n");

  uint32_t ss = 1024 * 5 * 2;
  void *fnstack = kmalloc_a(ss);
  create_task("sched_test", &sched_test_fn, 0, &sched_test_dfn, fnstack,
              fnstack, ss);
  int i = 1000000;
  while (1 != sem) {
    i--;
    if (i < 0) {
      assert("scheduler timed out");
    }
  }

  jmp_buf env = {0};

  if (setjmp(env) == 0) {
    longjmp(env, 42);
    kernel_printf("jump failed!\n");
  } else {
    kernel_printf("- jumps are good...\n");
  }
  serial_debug("selftest finished!");
}

vfs *init_vfs();
void vfs_print_current_tree(vfs *fs);

static zf_ctx *g_ctx = 0;

static void enter(const char *str) {
  if (g_ctx) {
    zf_result result = zf_eval(g_ctx, str);

    if (result == ZF_OK) {
      kernel_printf(" ok\n");
    } else {
      kernel_printf(" error %d\n", result);
    }

    kernel_printf("> ");
  }
}

static void bootstrap_zforth(zf_ctx *ctx, vfs *unified_vfs) {
  const char *forth_files[] = {"/fd/forth/dict.f", "/fd/forth/core.f", NULL};

  uint8_t buffer[1024] = {0};

  for (int file_idx = 0; forth_files[file_idx] != NULL; file_idx++) {
    const char *filepath = forth_files[file_idx];

    int fd = 0;
    if (VFS_SUCCESS !=
        unified_vfs->open(unified_vfs, filepath, VFS_READ, &fd)) {
      kernel_printf("file not found: %s\n", filepath);
      continue; // skip this file, try next one
    }

    memset(buffer, 0, sizeof(buffer));

    int bytes_read = 0;
    int ret = unified_vfs->read(unified_vfs, fd, buffer, sizeof(buffer) - 1,
                                buffer, &bytes_read);

    if (ret != VFS_SUCCESS) {
      kernel_printf("error %d reading %s\n", ret, filepath);
      unified_vfs->close(unified_vfs, fd);
      continue;
    }

    if (bytes_read > 0) {
      buffer[bytes_read] = '\0';
    }

    unified_vfs->close(unified_vfs, fd);

    zf_result result = zf_eval(ctx, (char *)buffer);
    if (result != ZF_OK) {
      kernel_printf("zforth error %d in file: %s\n", result, filepath);
    } else {
    }
  }

  kernel_printf("- zforth initialization complete\n");
}

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

  zf_ctx ctx;
  zf_init(&ctx, 1);
  zf_bootstrap(&ctx);
  bootstrap_zforth(&ctx, unified_vfs);

  keyboard_ctx_t *kb = get_kb_ctx();
  kb->enter_handler = enter;
  g_ctx = &ctx;
  kernel_printf(">");

  while (1) {
    ;
  }
}
