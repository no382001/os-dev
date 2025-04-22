#include "dhcp.h"
#include "libc/heap.h"
#include "libc/mem.h"
#include "libc/string.h"
#include "network.h"
#include "rtl8139.h"
#include "udp.h"

char ip_addr[4];
int is_ip_allocated;

/*
 * getter for IP address obtained from dhcp server
 * */
int gethostaddr(char *addr) {
  memcpy(addr, ip_addr, 4);
  if (!is_ip_allocated) {
    return 0;
  }
  return 1;
}

/*
 * broadcast a dhcp discover
 * */
void dhcp_discover() {
  uint8_t request_ip[4];
  uint8_t dst_ip[4];
  memset(request_ip, 0x0, 4);
  memset(dst_ip, 0xff, 4);
  dhcp_packet_t *packet = (dhcp_packet_t *)kmalloc(sizeof(dhcp_packet_t));
  memset(packet, 0, sizeof(dhcp_packet_t));
  make_dhcp_packet(packet, 1, request_ip);
  udp_send_packet(dst_ip, 68, 67, packet, sizeof(dhcp_packet_t));
}

/*
 * broadcast a dhcp request
 * */
// uint8_t??
void dhcp_request(uint8_t *request_ip) {
  uint8_t dst_ip[4];
  memset(dst_ip, 0xff, 4);
  dhcp_packet_t *packet = (dhcp_packet_t *)kmalloc(sizeof(dhcp_packet_t));
  memset(packet, 0, sizeof(dhcp_packet_t));
  make_dhcp_packet(packet, 3, request_ip);
  udp_send_packet(dst_ip, 68, 67, packet, sizeof(dhcp_packet_t));
}

/*
 * handle DHCP offer packet
 * */
void dhcp_handle_packet(dhcp_packet_t *packet) {
  // uint8_t *options = packet->options + 4;
  if (packet->op == DHCP_REPLY) {
    // DHCP Offer or ACK ?
    uint8_t *type = get_dhcp_options(packet, 53);
    if (*type == 2) {
      // offer, return a request
      dhcp_request((uint8_t *)&packet->your_ip); // uint8_t??
    } else if (*type == 5) {
      // ACK, save necessary info(IP for example)
      memcpy(ip_addr, &packet->your_ip, 4);
      is_ip_allocated = 1;
    }
  }
}

/*
 * search for the value of a type in options
 * */
void *get_dhcp_options(dhcp_packet_t *packet, uint8_t type) {
  uint8_t *options = packet->options + 4;
  uint8_t curr_type = *options;
  while (curr_type != 0xff) {
    uint8_t len = *(options + 1);
    if (curr_type == type) {
      // found type, return value
      void *ret = (void *)kmalloc(len);
      memcpy(ret, options + 2, len);
      return ret;
    }
    options += (2 + len);
  }
  return 0;
}

void make_dhcp_packet(dhcp_packet_t *packet, uint8_t msg_type,
                      uint8_t *request_ip) {
  packet->op = DHCP_REQUEST;
  packet->hardware_type = HARDWARE_TYPE_ETHERNET;
  packet->hardware_addr_len = 6;
  packet->hops = 0;
  packet->xid = htonl(DHCP_TRANSACTION_IDENTIFIER);
  packet->flags = htons(0x8000);
  get_mac_addr(packet->client_hardware_addr);

  // send dhcp packet using UDP
  uint8_t dst_ip[4];
  memset(dst_ip, 0xff, 4);

  // Ooptions specific to DHCP Discover (required)

  // magic cookie
  uint8_t *options = packet->options;
  *((uint32_t *)(options)) = htonl(0x63825363);
  options += 4;

  // first option, message type = DHCP_DISCOVER/DHCP_REQUEST
  *(options++) = 53;
  *(options++) = 1;
  *(options++) = msg_type;

  // client identifier
  *(options++) = 61;
  *(options++) = 0x07;
  *(options++) = 0x01;
  get_mac_addr(options);
  options += 6;

  // requested IP address
  *(options++) = 50;
  *(options++) = 0x04;
  *((uint32_t *)(options)) = htonl(0x0a00020e);
  memcpy((uint32_t *)(options), request_ip, 4);
  options += 4;

  // host Name
  *(options++) = 12;
  *(options++) = 0x09;
  memcpy(options, "simpleos", strlen("simpleos"));
  options += strlen("simpleos");
  *(options++) = 0x00;

  // parameter request list
  *(options++) = 55;
  *(options++) = 8;
  *(options++) = 0x1;
  *(options++) = 0x3;
  *(options++) = 0x6;
  *(options++) = 0xf;
  *(options++) = 0x2c;
  *(options++) = 0x2e;
  *(options++) = 0x2f;
  *(options++) = 0x39;
  *(options++) = 0xff;
}