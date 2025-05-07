#pragma once

#include "libc/types.h"

#define IP_IPV4 4

#define IP_PACKET_NO_FRAGMENT 2
#define IP_IS_LAST_FRAGMENT 4

#define PROTOCOL_UDP 17
#define PROTOCOL_TCP 6

typedef struct ip_packet {
  uint8_t version_ihl;      // version (4 bits) + IHL (4 bits)
  uint8_t tos;              // type of service
  uint16_t length;          // total length
  uint16_t id;              // identification
  uint16_t flags_fragment;  // flags (3 bits) + Fragment offset (13 bits)
  uint8_t ttl;              // time to live
  uint8_t protocol;         // protocol
  uint16_t header_checksum; // header checksum
  uint8_t src_ip[4];        // source IP address
  uint8_t dst_ip[4];        // destination IP address
  uint8_t data[];           // variable-length data
} __attribute__((packed)) ip_packet_t;

void get_ip_str(char *ip_str, uint8_t *ip);
uint16_t ip_calculate_checksum(ip_packet_t *packet);
void ip_send_packet(uint8_t *dst_ip, void *data, uint32_t len);
void ip_handle_packet(ip_packet_t *packet);