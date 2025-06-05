#pragma once
#include "libc/types.h"

#define BOOTREQUEST 1
#define DHCP_REPLY 2

#define DHCP_SERVER 67
#define DHCP_CLIENT 68

#define DHCP_DISCOVER 1
#define DHCP_REQUEST 3

#define DHCP_TRANSACTION_IDENTIFIER 0x55555555

// DHCP magic Cookie value
#define DHCP_MAGIC_COOKIE 0x63825363

// DHCP option codes
#define DHCP_OPT_PAD 0
#define DHCP_OPT_SUBNET_MASK 1
#define DHCP_OPT_ROUTER 3
#define DHCP_OPT_DNS 6
#define DHCP_OPT_HOST_NAME 12
#define DHCP_OPT_REQUESTED_IP 50
#define DHCP_OPT_MSG_TYPE 53
#define DHCP_OPT_CLIENT_ID 61
#define DHCP_OPT_PARAM_REQ_LIST 55
#define DHCP_OPT_END 255

// DHCP message types
#define DHCP_MSG_DISCOVER 1
#define DHCP_MSG_OFFER 2
#define DHCP_MSG_REQUEST 3
#define DHCP_MSG_ACK 5
#define DHCP_MSG_NAK 6

typedef struct dhcp_packet {
  uint8_t op;
  uint8_t hardware_type;
  uint8_t hardware_addr_len;
  uint8_t hops;
  uint32_t xid;
  uint16_t seconds;
  uint16_t flags;
  uint32_t client_ip;
  uint32_t your_ip;
  uint32_t server_ip;
  uint32_t gateway_ip;
  uint8_t client_hardware_addr[16];
  uint8_t server_name[64];
  uint8_t file[128];
  uint8_t options[308]; // increase this size to ensure enough space
} __attribute__((packed)) dhcp_packet_t;

int gethostaddr(char *addr);
void dhcp_discover();
void dhcp_request(uint8_t *requested_ip);
void dhcp_handle_packet(dhcp_packet_t *packet);
void *get_dhcp_options(dhcp_packet_t *packet, uint8_t type);
void make_dhcp_packet(dhcp_packet_t *packet, uint8_t msg_type,
                      uint8_t *request_ip);