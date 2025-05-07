#pragma once
#include "ip.h"

#define ICMP_ECHO_REPLY 0
#define ICMP_DEST_UNREACHABLE 3
#define ICMP_SOURCE_QUENCH 4
#define ICMP_REDIRECT 5
#define ICMP_ECHO_REQUEST 8
#define ICMP_TIME_EXCEEDED 11
#define ICMP_PARAMETER_PROBLEM 12

typedef struct icmp_packet {
  uint8_t type;
  uint8_t code;
  uint16_t checksum;
  uint16_t id;
  uint16_t sequence;
  uint8_t data[];
} __attribute__((packed)) icmp_packet_t;

void icmp_send_packet(uint8_t *dst_ip, uint8_t type, uint8_t code, uint16_t id,
                      uint16_t sequence, void *data, uint32_t data_len);

void icmp_handle_packet(icmp_packet_t *packet, uint16_t length,
                        uint8_t *src_ip);

void icmp_send_echo_request(uint8_t *dst_ip, uint16_t id, uint16_t sequence,
                            void *data, uint32_t data_len);