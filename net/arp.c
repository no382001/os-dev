#include "arp.h"
#include "drivers/serial.h"
#include "libc/heap.h"
#include "libc/mem.h"
#include "network.h"
#include "rtl8139.h"

arp_table_entry_t arp_table[512];
int arp_table_size;
int arp_table_curr;

uint8_t broadcast_mac_address[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

void arp_handle_packet(arp_packet_t *arp_packet) {

  uint8_t dst_hardware_addr[6];
  uint8_t dst_protocol_addr[4];
  // save some packet field
  memcpy(dst_hardware_addr, arp_packet->src_hardware_addr, 6);
  memcpy(dst_protocol_addr, arp_packet->src_protocol_addr, 4);
  // reply arp request, if the ip address matches(have to hard code the IP
  // eveywhere, because I don't have dhcp yet)
  if (ntohs(arp_packet->opcode) == ARP_REQUEST) {
    serial_debug("ARP_REQUEST......................");
    uint32_t my_ip = 0x0e02000a;
    if (memcmp(arp_packet->dst_protocol_addr, (uint8_t *)&my_ip, 4)) {

      // set source MAC address, IP address (hardcode the IP address as 10.2.2.3
      // until we really get one..)
      get_mac_addr(arp_packet->src_hardware_addr);
      arp_packet->src_protocol_addr[0] = 10;
      arp_packet->src_protocol_addr[1] = 0;
      arp_packet->src_protocol_addr[2] = 2;
      arp_packet->src_protocol_addr[3] = 14;

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

      // qemu_printf("Replied Arp, the reply looks like this\n");
    }
  } else if (ntohs(arp_packet->opcode) == ARP_REPLY) {
    return; // reply
  } else {
    return; // not for us
  }

  // now, store the ip-mac address mapping relation
  memcpy(&arp_table[arp_table_curr].ip_addr, dst_protocol_addr, 4);
  memcpy(&arp_table[arp_table_curr].mac_addr, dst_hardware_addr, 6);
  if (arp_table_size < 512)
    arp_table_size++;
  // wrap around
  if (arp_table_curr >= 512)
    arp_table_curr = 0;
}

void arp_send_packet(uint8_t *dst_hardware_addr, uint8_t *dst_protocol_addr) {
  arp_packet_t arp_packet = {0};

  // set source MAC address, IP address (hardcode the IP address as 10.2.2.3
  // until we really get one..)
  get_mac_addr(arp_packet.src_hardware_addr);
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

void arp_lookup_add(uint8_t *ret_hardware_addr, uint8_t *ip_addr) {
  memcpy(&arp_table[arp_table_curr].ip_addr, ip_addr, 4);
  memcpy(&arp_table[arp_table_curr].mac_addr, ret_hardware_addr, 6);
  if (arp_table_size < 512)
    arp_table_size++;
  // wrap around
  if (arp_table_curr >= 512)
    arp_table_curr = 0;
}

int arp_lookup(uint8_t *ret_hardware_addr, uint8_t *ip_addr) {
  uint32_t ip_entry = *((uint32_t *)(ip_addr));
  for (int i = 0; i < 512; i++) {
    if (arp_table[i].ip_addr == ip_entry) {
      memcpy(ret_hardware_addr, &arp_table[i].mac_addr, 6);
      return 1;
    }
  }
  return 0;
}

void arp_init() {
  uint8_t broadcast_ip[4];
  uint8_t broadcast_mac[6];

  memset(broadcast_ip, 0xff, 4);
  memset(broadcast_mac, 0xff, 6);
  arp_lookup_add(broadcast_mac, broadcast_ip);
}