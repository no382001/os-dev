
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

  kernel_printf("- initializing network driver...\n");
  rtl8139_init();
  serial_debug("rtl8139 done...");

  arp_init();
  serial_debug("arp done...");

  uint8_t mac_addr[] = {0};
  get_mac_addr(mac_addr);

  // this does not work i cant seem to set up the network on wsl2 qemu
  // the packet looks okay, but i cant reach the dhcp server

  // maybe put this in a state machine?
  dhcp_discover();
  extern int is_ip_allocated;
  extern int is_ip_offered;

  while (!is_ip_offered) {
  }

  while (!is_ip_allocated) {
    sleep(1000);
    extern uint32_t prev_requested_ip;
    dhcp_request((uint8_t *)&prev_requested_ip);
  }
  extern uint8_t my_ip[4];
  serial_debug("we got a dynamic ip!  %d.%d.%d.%d", my_ip[0], my_ip[1],
               my_ip[2], my_ip[3]);

  while (1) {
    ;
  }
}
