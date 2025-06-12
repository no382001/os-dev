#pragma once
#include "libc/types.h"

#define ETHERNET_TYPE_ARP 0x0806
#define ETHERNET_TYPE_IP 0x0800

#define HARDWARE_TYPE_ETHERNET 0x01

typedef struct ethernet_frame {
  uint8_t dst_mac_addr[6];
  uint8_t src_mac_addr[6];
  uint16_t type;
  uint8_t data[];
} __attribute__((
    packed)) ethernet_frame_t; // just give this down to evey handle_fn

int ethernet_send_packet(uint8_t *dst_mac_addr, uint8_t *data, int len,
                         uint16_t protocol);
void ethernet_handle_packet(ethernet_frame_t *packet, int len);

uint16_t flip_short(uint16_t short_int);
uint32_t flip_long(uint32_t long_int);
uint8_t flip_byte(uint8_t byte, int num_bits);
uint8_t htonb(uint8_t byte, int num_bits);
uint8_t ntohb(uint8_t byte, int num_bits);
uint16_t htons(uint16_t hostshort);
uint32_t htonl(uint32_t hostlong);
uint16_t ntohs(uint16_t netshort);
uint32_t ntohl(uint32_t netlong);