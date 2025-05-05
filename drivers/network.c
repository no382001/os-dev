#include "network.h"
#include "arp.h"
#include "ip.h"
#include "libc/heap.h"
#include "libc/mem.h"
#include "rtl8139.h"
#include "serial.h"

int ethernet_send_packet(uint8_t *dst_mac_addr, uint8_t *data, int len,
                         uint16_t protocol) {
  uint8_t src_mac_addr[6];
  ethernet_frame_t *frame =
      (ethernet_frame_t *)kmalloc(sizeof(ethernet_frame_t) + len);
  void *frame_data = (void *)((uint8_t *)frame + sizeof(ethernet_frame_t));

  // get source mac address from network card driver
  get_mac_addr(src_mac_addr);

  // fill in source and destination mac address
  memcpy(frame->src_mac_addr, src_mac_addr, 6);
  memcpy(frame->dst_mac_addr, dst_mac_addr, 6);

  // fill in data
  memcpy(frame_data, data, len);

  // fill in type
  frame->type = htons(protocol);

  rtl8139_send_packet(frame, sizeof(ethernet_frame_t) + len);
  kfree(frame);

  // qemu_printf("Sent an ethernet packet, it looks like this\n");

  return len;
}

void ethernet_handle_packet(ethernet_frame_t *packet, int len) {
  (void)len;
  void *data = (void *)(packet + sizeof(ethernet_frame_t));
  // ARP packet
  if (ntohs(packet->type) == ETHERNET_TYPE_ARP) {
    serial_debug("(ARP Packet)\n");
    arp_handle_packet(data);
    return;
  }
  // IP packets(could be TCP, UDP or others)
  if (ntohs(packet->type) == ETHERNET_TYPE_IP) {
    serial_debug("(IP Packet)\n");

    ip_handle_packet(data);
    return;
  }
  serial_debug("we dont know what to do with this packet %x",
               ntohs(packet->type));
}

uint16_t flip_short(uint16_t short_int) {
  uint32_t first_byte = *((uint8_t *)(&short_int));
  uint32_t second_byte = *((uint8_t *)(&short_int) + 1);
  return (first_byte << 8) | (second_byte);
}

uint32_t flip_long(uint32_t long_int) {
  uint32_t first_byte = *((uint8_t *)(&long_int));
  uint32_t second_byte = *((uint8_t *)(&long_int) + 1);
  uint32_t third_byte = *((uint8_t *)(&long_int) + 2);
  uint32_t fourth_byte = *((uint8_t *)(&long_int) + 3);
  return (first_byte << 24) | (second_byte << 16) | (third_byte << 8) |
         (fourth_byte);
}

// 0b11110000 will be 0b00001111 instead
uint8_t flip_byte(uint8_t byte, int num_bits) {
  uint8_t t = byte << (8 - num_bits);
  return t | (byte >> num_bits);
}

uint8_t htonb(uint8_t byte, int num_bits) { return flip_byte(byte, num_bits); }

uint8_t ntohb(uint8_t byte, int num_bits) {
  return flip_byte(byte, 8 - num_bits);
}

uint16_t htons(uint16_t hostshort) { return flip_short(hostshort); }

uint32_t htonl(uint32_t hostlong) { return flip_long(hostlong); }

uint16_t ntohs(uint16_t netshort) { return flip_short(netshort); }

uint32_t ntohl(uint32_t netlong) { return flip_long(netlong); }
