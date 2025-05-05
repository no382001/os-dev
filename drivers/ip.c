#include "ip.h"
#include "arp.h"
#include "dhcp.h"
#include "libc/string.h"
#include "network.h"
#include "serial.h"

uint8_t my_ip[] = {10, 0, 2, 14};
uint8_t test_target_ip[] = {10, 0, 2, 15};
uint8_t zero_hardware_addr[] = {0, 0, 0, 0, 0, 0};

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
  // treat the packet header as a 2-byte-integer array
  // sum all integers up and flip all bits
  int array_size = sizeof(ip_packet_t) / 2;
  uint32_t sum = 0;
  uint16_t value;

  uint8_t *byte_ptr = (uint8_t *)packet;

  for (int i = 0; i < array_size; i++) {
    memcpy(&value, &byte_ptr[i * 2], sizeof(uint16_t));
    sum += flip_short(value);
  }

  uint32_t carry = sum >> 16;
  sum = sum & 0x0000ffff;
  sum = sum + carry;
  uint16_t ret = ~sum;
  return ret;
}

void ip_send_packet(uint8_t *dst_ip, void *data, uint32_t len) {
  // Print what we're trying to send
  serial_debug("Sending IP packet to %d.%d.%d.%d with data length %d",
               dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3], len);

  // Allocate memory for the packet
  ip_packet_t *packet = (ip_packet_t *)kmalloc(sizeof(ip_packet_t) + len);

  // Fill in the IP header
  packet->version_ihl = (4 << 4) | 5; // IPv4, header length 5 words (20 bytes)
  serial_debug("version_ihl = 0x%02x", packet->version_ihl);

  packet->tos = 0;

  // Total length = header + data
  uint16_t total_length = sizeof(ip_packet_t) + len;
  packet->length = htons(total_length);
  serial_debug("total length = %d (%x in network order)", total_length,
               packet->length);

  // Set identification field
  static uint16_t ip_id = 0;
  packet->id = htons(ip_id++);

  // No fragmentation
  packet->flags_fragment = htons(0);

  // TTL and protocol
  packet->ttl = 64;
  packet->protocol = PROTOCOL_UDP; // Assuming UDP for DHCP

  uint8_t my_ip_address[4] = {0, 0, 0, 0};
  // Set source and destination addresses
  memcpy(packet->src_ip, my_ip_address, 4);
  memcpy(packet->dst_ip, dst_ip, 4);

  // Calculate checksum
  packet->header_checksum = 0; // Must be zero for calculation
  packet->header_checksum =
      ip_calculate_checksum(packet); // Standard IP header is 20 bytes
  serial_debug("IP checksum = 0x%04x", ntohs(packet->header_checksum));

  // Copy the data
  memcpy(packet->data, data, len);

  // Get the MAC address for the destination IP
  uint8_t dst_mac[6];
  if (arp_lookup(dst_ip, dst_mac) != 0) {
    serial_debug("No MAC found for IP %d.%d.%d.%d - sending ARP request",
                 dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3]);
    // Send ARP request
    uint8_t broadcast_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    arp_send_packet(broadcast_mac, dst_ip);
    // We could queue the packet here for later transmission
    kfree(packet);
    return;
  }

  // Send Ethernet frame with the IP packet
  serial_debug("Sending to MAC %02x:%02x:%02x:%02x:%02x:%02x", dst_mac[0],
               dst_mac[1], dst_mac[2], dst_mac[3], dst_mac[4], dst_mac[5]);

  ethernet_send_packet(dst_mac, (void *)packet, total_length, ETHERNET_TYPE_IP);

  kfree(packet);
}

void ip_handle_packet(ip_packet_t *packet) {
  // fix packet data order (be careful with the endiness problem within a byte)
  uint8_t *version_ihl_byte = (uint8_t *)packet;
  *version_ihl_byte = ntohb(*version_ihl_byte, 4);

  uint8_t *flags_fragment_byte = (uint8_t *)packet + 6;
  *flags_fragment_byte = ntohb(*flags_fragment_byte, 3);

  serial_printff("Receive: the whole ip packet \n");
  // now, the ip packet handler simply dumps ip header info and the data with
  // xxd and display on screen Dump source ip, data, checksum
  char src_ip[20];

  if ((packet->version_ihl >> 4) == IP_IPV4) {
    get_ip_str(src_ip, packet->src_ip);

    // header size from ihl field
    void *data_ptr =
        (void *)((uint8_t *)packet + ((packet->version_ihl & 0x0F) * 4));
    // int data_len = ntohs(packet->length) - sizeof(ip_packet_t);

    serial_printff("src: %s, data dump: \n", src_ip);
    // xxd(data_ptr, data_len);

    // If this is a UDP packet
    if (packet->protocol == PROTOCOL_UDP) {
      udp_handle_packet(data_ptr);
    }

    // TODO
    // What ? that's it ? that's ip packet handling ??
    // not really... u need to handle ip fragmentation... but let's make sure we
    // can handle one ip packet first
  }
}