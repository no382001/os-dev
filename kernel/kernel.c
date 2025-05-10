
#include "bits.h"

void selftest(void);
void fvm_test(void);

semaphore_t task_semaphore = {0};

void kernel_main(void) {
  serial_debug("starting up...");
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

  kernel_printf("- initializing network driver...\n");
  rtl8139_init();
  serial_debug("rtl8139 done...");

  arp_init();
  serial_debug("arp done...");

  uint8_t mac_addr[] = {0};
  get_mac_addr(mac_addr);
  /*
   */

  /*
  ethernet_send_packet(mac_addr, (void *)str, strlen(str), 0x0021);
  ip_send_packet(ip_addr, (void *)str, strlen(str));
  */

  // this does not work i cant seem to set up the network on wsl2 qemu
  // the packet looks okay, but i cant reach the dhcp server
  /*
  dhcp_discover();
  while (gethostaddr((char *)mac_addr) == 0){};
  */

  // udp_send_packet(ip_addr, 1234, 1153, str, strlen(str));
  for (;;)
    ;
}
