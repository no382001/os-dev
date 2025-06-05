#include "ip.h"
#include "arp.h"
#include "dhcp.h"
#include "drivers/serial.h"
#include "libc/string.h"
#include "network.h"

/*
 */
#undef serial_debug
#define serial_debug(...)

uint8_t my_ip[] = {0, 0, 0, 0};

void get_ip_str(char *ip_str, uint8_t *ip) {
  char temp[4];

  ip_str[0] = '\0';

  itoa(ip[0], temp); // ?
  strcat(ip_str, temp);

  for (int i = 1; i < 4; i++) {
    strcat(ip_str, ".");
    itoa(ip[i], temp);
    strcat(ip_str, temp);
  }
}
uint16_t ip_calculate_checksum(ip_packet_t *packet) {
  uint32_t sum = 0;
  uint8_t *byte_ptr = (uint8_t *)packet;
  int header_len = 20; // IP header is always 20 bytes for basic IPv4

  // sum all 16-bit words in the header
  for (int i = 0; i < header_len; i += 2) {
    uint16_t word = (byte_ptr[i] << 8) | byte_ptr[i + 1];
    sum += word;
  }

  // add carry bits and fold to 16 bits
  while (sum >> 16) {
    sum = (sum & 0xFFFF) + (sum >> 16);
  }

  // return ones complement
  return htons(~sum);
}

void ip_send_packet(uint8_t *dst_ip, void *data, uint32_t len) {
  _ip_send_packet(dst_ip, data, len, PROTOCOL_UDP); // Assuming UDP for DHCP
}

void _ip_send_packet(uint8_t *dst_ip, void *data, uint32_t len,
                     uint8_t protocol) {
  serial_debug("sending ip packet to %d.%d.%d.%d with data length %d",
               dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3], len);

  ip_packet_t *packet = (ip_packet_t *)kmalloc(sizeof(ip_packet_t) + len);

  packet->version_ihl =
      (IP_IPV4 << 4) | 5; // IPv4, header length 5 words (20 bytes)
  serial_debug("version_ihl = %x", packet->version_ihl);

  packet->tos = 0;

  uint16_t total_length = sizeof(ip_packet_t) + len;
  packet->length = htons(total_length);
  // serial_debug("total length = %d (%x in network order)", total_length,
  // packet->length);

  static uint16_t ip_packet_id = 0;

  packet->id = htons(ip_packet_id++);

  // DF flag = 0x4000
  packet->flags_fragment = htons(0x4000);

  // TTL and protocol
  packet->ttl = 64;
  packet->protocol = protocol;

  memcpy(packet->src_ip, my_ip, 4);
  memcpy(packet->dst_ip, dst_ip, 4);

  packet->header_checksum = 0;
  packet->header_checksum = ip_calculate_checksum(packet);
  // serial_debug("ip checksum = %x", ntohs(packet->header_checksum));

  memcpy(packet->data, data, len);

  uint8_t dst_mac[6] = {0};
  if (arp_lookup(dst_mac, dst_ip) == 0) {
    serial_debug("no MAC found for IP %d.%d.%d.%d - sending ARP request",
                 dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3]);

    uint8_t broadcast_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    arp_send_packet(broadcast_mac, dst_ip);
    // TODO we could queue the packet here for later transmission
    // cuz we miss a packet now
    // also i should add the mac to the lut when it arrives
    kfree(packet);
    return;
  }
  // arp_print_table();
  serial_debug("sending to mac %x:%x:%x:%x:%x:%x", dst_mac[0], dst_mac[1],
               dst_mac[2], dst_mac[3], dst_mac[4], dst_mac[5]);

  ethernet_send_packet(dst_mac, (void *)packet, total_length, ETHERNET_TYPE_IP);

  kfree(packet);
}

void ip_handle_packet(ethernet_frame_t *frame) {

  ip_packet_t *packet = (ip_packet_t *)frame->data;

  {
    uint8_t _[6] = {0};
    if (arp_lookup(_, packet->src_ip) == 0) {
      arp_lookup_add(frame->src_mac_addr, packet->src_ip);
    }
  }

  serial_debug("got ip packet w/ type %x ver %x", packet->protocol,
               (packet->version_ihl >> 4));

  char src_ip[20];

  if ((packet->version_ihl >> 4) == IP_IPV4) {
    get_ip_str(src_ip, packet->src_ip);

    // header size from ihl field
    void *data_ptr =
        (void *)((uint8_t *)packet + ((packet->version_ihl & 0x0F) * 4));
    int data_len = ntohs(packet->length) - sizeof(ip_packet_t);

    if (packet->protocol == PROTOCOL_UDP) {
      udp_handle_packet(data_ptr);
    } else if (packet->protocol == PROTOCOL_ICMP) {
      icmp_handle_packet((icmp_packet_t *)data_ptr, data_len, packet->src_ip);
    }

    // TODO
    // What ? that's it ? that's ip packet handling ??
    // not really... u need to handle ip fragmentation... but let's make sure we
    // can handle one ip packet first
  } else {
    uint8_t *bytes = (uint8_t *)packet;
    serial_debug("we dont know what this is: %02x %02x %02x %02x %02x %02x",
                 bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5]);
  }
}