#include "arp.h"
#include "drivers/serial.h"
#include "libc/heap.h"
#include "libc/mem.h"
#include "network.h"
#include "rtl8139.h"

/*
 */
#undef serial_debug
#define serial_debug(...)

#define ARP_TABLE_MAX 16
arp_table_entry_t arp_table[ARP_TABLE_MAX] = {0};
int arp_table_curr = 0;

uint8_t broadcast_mac_address[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

extern uint8_t my_ip[4];

void arp_handle_packet(arp_packet_t *arp_packet) {

  uint8_t dst_hardware_addr[6];
  uint8_t dst_protocol_addr[4];
  // save some packet field
  memcpy(dst_hardware_addr, arp_packet->src_hardware_addr, 6);
  memcpy(dst_protocol_addr, arp_packet->src_protocol_addr, 4);

  if (ntohs(arp_packet->opcode) == ARP_REQUEST) {
    if (!memcmp(arp_packet->dst_protocol_addr, my_ip, 4)) {

      get_mac_addr(arp_packet->src_hardware_addr);
      memcpy(arp_packet->src_protocol_addr, my_ip, 4);

      // set destination MAC address, IP address
      memcpy(arp_packet->dst_hardware_addr, dst_hardware_addr, 6);
      memcpy(arp_packet->dst_protocol_addr, dst_protocol_addr, 4);

      // set opcode
      arp_packet->opcode = htons(ARP_REPLY);

      // set lengths
      arp_packet->hardware_addr_len = 6;
      arp_packet->protocol_addr_len = 4;

      // set hardware type
      arp_packet->hardware_type = htons(HARDWARE_TYPE_ETHERNET);

      // set protocol = IPv4
      arp_packet->protocol = htons(ETHERNET_TYPE_IP);

      // now send it with ethernet
      ethernet_send_packet(dst_hardware_addr, (uint8_t *)arp_packet,
                           sizeof(arp_packet_t), ETHERNET_TYPE_ARP);
      serial_debug(
          "sent an arp reply to %d.%d.%d.%d %x:%x:%x:%x",
          arp_packet->dst_protocol_addr[0], arp_packet->dst_protocol_addr[1],
          arp_packet->dst_protocol_addr[2], arp_packet->dst_protocol_addr[3],
          arp_packet->dst_hardware_addr[0], arp_packet->dst_hardware_addr[1],
          arp_packet->dst_hardware_addr[2], arp_packet->dst_hardware_addr[3]);
    }
  } else if (ntohs(arp_packet->opcode) == ARP_REPLY) {
    serial_debug("arp reply");
    // reply
  } else {
    serial_debug("arp not for us");
    return; // not for us
  }
  uint8_t _[6] = {0};
  if (arp_lookup(_, arp_packet->src_protocol_addr) == 0) {
    arp_lookup_add(arp_packet->src_hardware_addr,
                   arp_packet->src_protocol_addr);
  }
}

void arp_send_packet(uint8_t *dst_hardware_addr, uint8_t *dst_protocol_addr) {
  arp_packet_t arp_packet = {0};

  // set source MAC address, IP address (hardcode the IP address as 10.2.2.3
  // until we really get one..)
  get_mac_addr(arp_packet.src_hardware_addr);
  // TODO
  arp_packet.src_protocol_addr[0] = 10;
  arp_packet.src_protocol_addr[1] = 0;
  arp_packet.src_protocol_addr[2] = 2;
  arp_packet.src_protocol_addr[3] = 14;

  // set destination MAC address, IP address
  memcpy(arp_packet.dst_hardware_addr, dst_hardware_addr, 6);
  memcpy(arp_packet.dst_protocol_addr, dst_protocol_addr, 4);

  // set opcode
  arp_packet.opcode = htons(ARP_REQUEST);

  // set lengths
  arp_packet.hardware_addr_len = 6;
  arp_packet.protocol_addr_len = 4;

  // set hardware type
  arp_packet.hardware_type = htons(HARDWARE_TYPE_ETHERNET);

  // set protocol = IPv4
  arp_packet.protocol = htons(ETHERNET_TYPE_IP);

  // now send it with ethernet
  ethernet_send_packet(broadcast_mac_address, (uint8_t *)&arp_packet,
                       sizeof(arp_packet_t), ETHERNET_TYPE_ARP);
}

void arp_lookup_add(uint8_t *hardware_addr, uint8_t *ip_addr) {
  memcpy(&arp_table[arp_table_curr].ip_addr, ip_addr, 4);
  memcpy(&arp_table[arp_table_curr].mac_addr, hardware_addr, 6);

  serial_debug("new entry in arp table: %d.%d.%d.%d %x:%x:%x:%x:%x:%x",
               ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3], hardware_addr[0],
               hardware_addr[1], hardware_addr[2], hardware_addr[3],
               hardware_addr[4], hardware_addr[5]);

  arp_table_curr++;
  // wrap around
  if (arp_table_curr >= ARP_TABLE_MAX)
    arp_table_curr = 0;
}

// supply a destination mac and an ip to search
// 0 not found
int arp_lookup(uint8_t *ret_hardware_addr, uint8_t *ip_addr) {
  for (int i = 0; i < ARP_TABLE_MAX; i++) {
    if (!memcmp((uint8_t *)&arp_table[i].ip_addr, ip_addr, 4)) {
      memcpy(ret_hardware_addr, &arp_table[i].mac_addr, 6);
      return 1;
    }
  }
  return 0;
}

void arp_init() {
  uint8_t broadcast_ip[4] = {0xff, 0xff, 0xff, 0xff};
  uint8_t broadcast_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

  arp_lookup_add(broadcast_mac, broadcast_ip);
}
