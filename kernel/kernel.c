
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

  // selftest();

  kernel_printf("- initializing pci...\n");
  pci_init();
  serial_debug("pci done...");

  // set_vga_mode12();
  // init_vga12h();
  // vga_demo_terminal();
  // vga12h_gradient_demo();

  kernel_printf("- initializing network driver...\n");

  rtl8139_init();
  serial_debug("rtl8139 done...");

  arp_init();
  serial_debug("arp done...");

  uint8_t mac_addr[6];
  mac_addr[0] = 0xAA;
  mac_addr[1] = 0xBB;
  mac_addr[2] = 0xCC;
  mac_addr[3] = 0xDD;
  mac_addr[4] = 0xEE;
  mac_addr[5] = 0xFF;
  get_mac_addr(mac_addr);
  serial_debug("copied mac addr...");

  uint8_t ip_addr[6];
  ip_addr[0] = 10;
  ip_addr[1] = 0;
  ip_addr[2] = 2;
  ip_addr[3] = 15;
  char *str = "this is a message sent from the OS";
  // ethernet_send_packet(mac_addr, str, strlen(str), 0x0021);
  // ip_send_packet(ip_addr, str, strlen(str));

  kernel_printf("- asking for an ip...\n");
  serial_debug("dhcp discover...");
  dhcp_discover();
  // If IP is ready, send a UDP message from simpleos to a host machine running
  // Ubuntu
  serial_debug("looking for our ip...");
  while (gethostaddr((char *)mac_addr) == 0)
    ;
  serial_debug("sending dummy...");
  udp_send_packet(ip_addr, 1234, 1153, str, strlen(str));
  for (;;)
    ;
}
