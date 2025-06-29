
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
  serial_debug("selftest finished!");
}

void demo();
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

  fat_bpb_t bpb = {0};
  fat16_read_bpb(&bpb);

  fs_node_t *root = fs_build_root(&bpb);
  fs_print_tree_list(root);

  vfs fat_16_vfs = {0};
  fat16_vfs_data usercode = {0};
  usercode.root = root;
  usercode.current_dir = root;
  usercode.bpb = &bpb;

  vfs_init_fat16(&fat_16_vfs, &usercode);

  fat_16_vfs.chdir(&fat_16_vfs, "FONTS");
  int fd = 0;
  fat_16_vfs.open(&fat_16_vfs, "VIII.BDF", VFS_READ, &fd);

  fat_16_vfs.chdir(&fat_16_vfs, "/");

  vfs_stat s = {0};
  fat_16_vfs.stat(&fat_16_vfs, "/FONTS/VIII.BDF", &s);

  int r = fat_16_vfs.close(&fat_16_vfs, fd);

  fat_16_vfs.open(&fat_16_vfs, "/", VFS_READ, &fd);

  vfs_stat entries[10];
  int64_t count = fat_16_vfs.readdir(&fat_16_vfs, fd, entries, 10);
  for (int i = 0; i < count; i++) {
    kernel_printf("found: %s (%d bytes) %s\n", entries[i].name, entries[i].size,
                  entries[i].type == FS_TYPE_DIRECTORY ? "[DIR]" : "");
  }

  fat_16_vfs.close(&fat_16_vfs, fd);

  if (VFS_SUCCESS != fat_16_vfs.open(&fat_16_vfs, "file.txt", VFS_READ, &fd)) {
    kernel_printf("file not found!");
  } else {
    uint8_t buffer[512];
    int c = 0;
    int ret = 0;
    ret = fat_16_vfs.read(&fat_16_vfs, fd, buffer, sizeof(buffer) - 1, buffer,
                          &c);
    if (ret != VFS_SUCCESS) {
      kernel_printf("vfs error %d", ret);
    } else {
      if (c > 0) {
        buffer[c] = '\0';
        kernel_printf("file contents:\n%s\n", buffer);
      }
    }

    fat_16_vfs.close(&fat_16_vfs, fd);
  }

  while (1) {
    ;
  }
}
