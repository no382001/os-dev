#include "rtl8139.h"

#include "drivers/low_level.h"
#include "drivers/pci.h"
#include "libc/string.h"

pci_dev_t pci_rtl8139_device;
rtl8139_dev_t rtl8139_device;

uint32_t current_packet_ptr = 0;

// four TXAD register, you must use a different one to send packet each time(for
// example, use the first one, second... fourth and back to the first)
uint8_t TSAD_array[4] = {0x20, 0x24, 0x28, 0x2C};
uint8_t TSD_array[4] = {0x10, 0x14, 0x18, 0x1C};

void receive_packet() {
  uint16_t *t = (uint16_t *)(rtl8139_device.rx_buffer + current_packet_ptr);
  // skip packet header, get packet length
  uint16_t packet_length = *(t + 1);

  // skip, packet header and packet length, now t points to the packet data
  t = t + 2;

  // now, ethernet layer starts to handle the packet(be sure to make a copy of
  // the packet, insteading of using the buffer) and probabbly this should be
  // done in a separate thread...
  void *packet = (void *)kmalloc(packet_length);
  memcpy(packet, t, packet_length);
  ethernet_handle_packet(packet, packet_length);

  current_packet_ptr =
      (current_packet_ptr + packet_length + 4 + 3) & RX_READ_POINTER_MASK;

  if (current_packet_ptr > RX_BUF_SIZE)
    current_packet_ptr -= RX_BUF_SIZE;

  port_word_out(rtl8139_device.io_base + CAPR, current_packet_ptr - 0x10);
}

void rtl8139_handler(registers_t *reg) {
  (void)reg;
  uint16_t status = port_word_in(rtl8139_device.io_base + 0x3e);

  if (status & TOK) {
    serial_debug("packet sent");
  }
  if (status & ROK) {
    serial_debug("packet received");
    receive_packet();
  }
  // so the problem seems to be that the rtl8139_handler is triggered multiple
  // times, bc of the scheduler but its cli, inside the irq interrupt no? how
  // could that be
  // i reenable with prinf or whatever, fuck!
  // so i need a specialized cli/sti for when im in the interrupt

  port_word_out(rtl8139_device.io_base + 0x3E, 0x5);
}

void read_mac_addr() {
  uint32_t mac_part1 = port_long_in(rtl8139_device.io_base + 0x00);
  uint16_t mac_part2 = port_word_in(rtl8139_device.io_base + 0x04);
  rtl8139_device.mac_addr[0] = mac_part1 >> 0;
  rtl8139_device.mac_addr[1] = mac_part1 >> 8;
  rtl8139_device.mac_addr[2] = mac_part1 >> 16;
  rtl8139_device.mac_addr[3] = mac_part1 >> 24;

  rtl8139_device.mac_addr[4] = mac_part2 >> 0;
  rtl8139_device.mac_addr[5] = mac_part2 >> 8;
  serial_debug("our mac is: %x:%x:%x:%x:%x:%x", rtl8139_device.mac_addr[0],
               rtl8139_device.mac_addr[1], rtl8139_device.mac_addr[2],
               rtl8139_device.mac_addr[3], rtl8139_device.mac_addr[4],
               rtl8139_device.mac_addr[5]);
}

void get_mac_addr(uint8_t *src_mac_addr) {
  memcpy(src_mac_addr, rtl8139_device.mac_addr, 6);
}

void rtl8139_send_packet(void *data, uint32_t len) {
  // first, copy the data to a physically contiguous chunk of memory
  void *transfer_data = (void *)kmalloc_a(len);
  void *phys_addr = (void *)virt2phys(transfer_data);
  memcpy(transfer_data, data, len);

  // second, fill in physical address of data, and length
  port_long_out(rtl8139_device.io_base + TSAD_array[rtl8139_device.tx_cur],
                (uint32_t)phys_addr);
  port_long_out(rtl8139_device.io_base + TSD_array[rtl8139_device.tx_cur++],
                len);
  if (rtl8139_device.tx_cur > 3)
    rtl8139_device.tx_cur = 0;
}

void rtl8139_init() {
  // first get the network device using PCI
  pci_rtl8139_device = pci_get_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID, -1);
  uint32_t ret = pci_read(pci_rtl8139_device, PCI_BAR0);

  rtl8139_device.bar_type = ret & 0x1;
  // get io base or mem base by extracting the high 28/30 bits
  rtl8139_device.io_base = ret & (~0x3);
  rtl8139_device.mem_base = ret & (~0xf);
  serial_debug("rtl8139 use %s access (base: %x)",
               (rtl8139_device.bar_type == 0) ? "mem based" : "port based",
               (rtl8139_device.bar_type != 0) ? rtl8139_device.io_base
                                              : rtl8139_device.mem_base);

  // set current TSAD
  rtl8139_device.tx_cur = 0;

  // enable PCI Bus Mastering
  uint32_t pci_command_reg = pci_read(pci_rtl8139_device, PCI_COMMAND);
  if (!(pci_command_reg & (1 << 2))) {
    pci_command_reg |= (1 << 2);
    pci_write(pci_rtl8139_device, PCI_COMMAND, pci_command_reg);
  }

  // send 0x00 to the CONFIG_1 register (0x52) to set the LWAKE + LWPTN to
  // active high. this should essentially *power on* the device.
  port_byte_out(rtl8139_device.io_base + 0x52, 0x0);

  // soft reset
  port_byte_out(rtl8139_device.io_base + 0x37, 0x10);
  while ((port_byte_in(rtl8139_device.io_base + 0x37) & 0x10) != 0) {
  }

  // allocate receive buffer
  rtl8139_device.rx_buffer = (char *)kmalloc(8192 + 16 + 1500);
  memset(rtl8139_device.rx_buffer, 0x0, 8192 + 16 + 1500);

  port_long_out(rtl8139_device.io_base + 0x30,
                (uint32_t)virt2phys(rtl8139_device.rx_buffer));

  // sets the TOK and ROK bits high
  port_word_out(rtl8139_device.io_base + 0x3C, 0x0005);

  // (1 << 7) is the WRAP bit, 0xf is AB+AM+APM+AAP
  port_long_out(rtl8139_device.io_base + 0x44, 0xf | (1 << 7));

  // sets the RE and TE bits high
  port_byte_out(rtl8139_device.io_base + 0x37, 0x0C);

  // register and enable network interrupts
  uint32_t irq_num = pci_read(pci_rtl8139_device, PCI_INTERRUPT_LINE);
  register_interrupt_handler(32 + irq_num, rtl8139_handler);
  kernel_printf("[] rtl8139 registered on irq = %d\n", irq_num);

  read_mac_addr();
}