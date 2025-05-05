#include "udp.h"
#include "dhcp.h"
#include "ip.h"
#include "libc/heap.h"
#include "libc/mem.h"
#include "network.h"
#include "serial.h"

uint16_t udp_calculate_checksum(udp_packet_t *packet) {
  (void)packet;
  // UDP checksum is optional in IPv4
  return 0;
}

void udp_send_packet(uint8_t *dst_ip, uint16_t src_port, uint16_t dst_port,
                     void *data, int len) {
  int length = sizeof(udp_packet_t) + len;
  serial_debug(
      "UDP send: dst_ip=%d.%d.%d.%d, src_port=%d, dst_port=%d, length=%d",
      dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3], src_port, dst_port, length);

  udp_packet_t *packet = (udp_packet_t *)kmalloc(length);
  memset(packet, 0, sizeof(udp_packet_t));

  packet->src_port = htons(src_port);
  packet->dst_port = htons(dst_port);
  packet->length = htons(length); // or add 8 for the header??
  packet->checksum = udp_calculate_checksum(packet);

  // copy data over
  memcpy(packet->data, data, len);
  // serial_debug("udp packet sent");
  ip_send_packet(dst_ip, packet, length);
}

void udp_handle_packet(udp_packet_t *packet) {
  // uint16_t src_port = ntohs(packet->src_port);
  uint16_t dst_port = ntohs(packet->dst_port);
  // uint16_t length = ntohs(packet->length);

  void *data_ptr = (void *)(packet + sizeof(udp_packet_t));
  // uint32_t data_len = length;
  serial_debug("Received UDP packet, dst_port %d, data dump:\n", dst_port);
  // xxd(data_ptr, data_len);

  if (ntohs(packet->dst_port) == DHCP_CLIENT) {
    dhcp_handle_packet(data_ptr);
  }
}