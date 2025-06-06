#include "dhcp.h"
#include "libc/heap.h"
#include "libc/mem.h"
#include "libc/string.h"
#include "network.h"
#include "rtl8139.h"
#include "udp.h"

/*
 */
#undef serial_debug
#define serial_debug(...)

extern uint8_t my_ip[4];
int is_ip_allocated = 0;
int is_ip_offered = 0;
uint32_t dhcp_server_ip = 0;
uint32_t prev_requested_ip = 0;

int gethostaddr(char *addr) {
  memcpy(addr, my_ip, 4);
  return is_ip_allocated;
}

uint8_t *add_dhcp_option(uint8_t *options, uint8_t option, uint8_t len,
                         void *data) {
  *(options++) = option;
  *(options++) = len;
  memcpy(options, data, len);
  return options + len;
}

void dhcp_discover() {
  serial_debug("dhcp discover...");
  uint8_t dst_ip[4] = {255, 255, 255, 255};

  dhcp_packet_t packet = {0};

  packet.op = BOOTREQUEST;
  packet.hardware_type = HARDWARE_TYPE_ETHERNET;
  packet.hardware_addr_len = 6;
  packet.hops = 0;
  packet.xid = htonl(DHCP_TRANSACTION_IDENTIFIER);
  packet.flags = htons(0x8000); // broadcast flag
  get_mac_addr(packet.client_hardware_addr);

  uint8_t *options = packet.options;

  *((uint32_t *)(options)) = htonl(DHCP_MAGIC_COOKIE);
  options += 4;

  uint8_t msg_type = DHCP_DISCOVER;
  options = add_dhcp_option(options, DHCP_OPT_MSG_TYPE, 1, &msg_type);

  uint8_t client_id[7];
  client_id[0] = 1; // hardware type (Ethernet)
  get_mac_addr(client_id + 1);
  options = add_dhcp_option(options, DHCP_OPT_CLIENT_ID, 7, client_id);

  char hostname[] = "os-dev";
  options =
      add_dhcp_option(options, DHCP_OPT_HOST_NAME, strlen(hostname), hostname);

  uint8_t params[] = {
      DHCP_OPT_SUBNET_MASK, // subnet mask
      DHCP_OPT_ROUTER,      // router
      DHCP_OPT_DNS,         // DNS server
      15,                   // domain name
      44,                   // NETBIOS name server
      46,                   // NETBIOS node type
      47,                   // NETBIOS scope
      57                    // maximum DHCP message size
  };
  options =
      add_dhcp_option(options, DHCP_OPT_PARAM_REQ_LIST, sizeof(params), params);
  *(options++) = DHCP_OPT_END;

  udp_send_packet(dst_ip, DHCP_CLIENT, DHCP_SERVER, &packet,
                  sizeof(dhcp_packet_t));
}

void dhcp_request(uint8_t *requested_ip) {
  uint8_t dst_ip[4] = {255, 255, 255, 255}; // need to broadcast
  dhcp_packet_t packet = {0};

  packet.op = BOOTREQUEST;
  packet.hardware_type = HARDWARE_TYPE_ETHERNET;
  packet.hardware_addr_len = 6;
  packet.hops = 0;
  packet.xid = htonl(DHCP_TRANSACTION_IDENTIFIER);
  packet.flags = htons(0x8000); // broadcast flag

  get_mac_addr(packet.client_hardware_addr);

  uint8_t *options = packet.options;

  // magic cookie
  *((uint32_t *)(options)) = htonl(DHCP_MAGIC_COOKIE);
  options += 4;

  // message type (REQUEST)
  uint8_t msg_type = DHCP_REQUEST;
  options = add_dhcp_option(options, DHCP_OPT_MSG_TYPE, 1, &msg_type);

  // client identifier
  uint8_t client_id[7];
  client_id[0] = 0x1; // hardware type (Ethernet)
  get_mac_addr(client_id + 1);
  options = add_dhcp_option(options, DHCP_OPT_CLIENT_ID, 7, client_id);

  if (requested_ip) {
    options = add_dhcp_option(options, DHCP_OPT_REQUESTED_IP, 4, requested_ip);
  }

  // server identifier (if we have it)
  if (dhcp_server_ip != 0) {
    options =
        add_dhcp_option(options, 54, 4, &dhcp_server_ip); // 54 = Server ID
  }

  // host name (optional)
  char hostname[] = "os-dev";
  options =
      add_dhcp_option(options, DHCP_OPT_HOST_NAME, strlen(hostname), hostname);

  // parameter request list
  uint8_t params[] = {
      DHCP_OPT_SUBNET_MASK, // subnet mask
      DHCP_OPT_ROUTER,      // router
      DHCP_OPT_DNS          // DNS server
  };

  options =
      add_dhcp_option(options, DHCP_OPT_PARAM_REQ_LIST, sizeof(params), params);

  *(options++) = DHCP_OPT_END;

  udp_send_packet(dst_ip, DHCP_CLIENT, DHCP_SERVER, &packet,
                  sizeof(dhcp_packet_t));
}

void *get_dhcp_option(dhcp_packet_t *packet, uint8_t type) {
  if (ntohl(*(uint32_t *)packet->options) != DHCP_MAGIC_COOKIE) {
    return NULL;
  }

  uint8_t *options = packet->options + 4; // skip magic cookie

  while (options < packet->options + sizeof(packet->options)) {
    uint8_t curr_type = *options;

    if (curr_type == DHCP_OPT_END) {
      break;
    }

    if (curr_type == DHCP_OPT_PAD) {
      options++;
      continue;
    }

    uint8_t len = *(options + 1);

    if (curr_type == type) {
      return options + 2;
    }

    options += (2 + len);
  }

  return NULL;
}

void dhcp_handle_packet(dhcp_packet_t *packet) {
  if (packet->op != DHCP_REPLY) {
    serial_debug("not a reply! its %x", packet->op);
    return;
  }

  if (ntohl(packet->xid) != DHCP_TRANSACTION_IDENTIFIER) {
    serial_debug("not our xid!");
    return;
  }

  uint8_t *type_ptr = get_dhcp_option(packet, DHCP_OPT_MSG_TYPE);
  if (!type_ptr) {
    serial_debug("invalid msg type!");
    return;
  }

  uint8_t msg_type = *type_ptr;

  switch (msg_type) {
  case DHCP_MSG_OFFER: {
    is_ip_offered = 1;
    memcpy(&prev_requested_ip, &packet->your_ip, 4);
    serial_debug("offer! of %d.%d.%d.%d", ((uint8_t *)&packet->your_ip)[0],
                 ((uint8_t *)&packet->your_ip)[1],
                 ((uint8_t *)&packet->your_ip)[2],
                 ((uint8_t *)&packet->your_ip)[3]);

    uint8_t *server_id = get_dhcp_option(packet, 54); // 54 = Server ID
    if (server_id) {
      memcpy(&dhcp_server_ip, server_id, 4);
      serial_debug("dhcp server: %d.%d.%d.%d", server_id[0], server_id[1],
                   server_id[2], server_id[3]);
    }

    dhcp_request((uint8_t *)&packet->your_ip);
  } break;

  case DHCP_MSG_ACK:
    if (is_ip_allocated) {
      break;
    }
    memcpy(my_ip, &packet->your_ip, 4);
    is_ip_allocated = 1;
    serial_debug("we got an ip!");

    // process other configuration options (subnet, router, DNS)
    uint8_t *subnet_mask = get_dhcp_option(packet, DHCP_OPT_SUBNET_MASK);
    uint8_t *router = get_dhcp_option(packet, DHCP_OPT_ROUTER);
    uint8_t *dns = get_dhcp_option(packet, DHCP_OPT_DNS);

    // configure the network with these values (implementation needed)
    if (subnet_mask) {
      // set_subnet_mask(subnet_mask);
    }

    if (router) {
      // set_default_gateway(router);
    }

    if (dns) {
      // set_dns_server(dns);
    }
    break;

  case DHCP_MSG_NAK:
    serial_debug("nack!");
    // start over with DISCOVER
    is_ip_allocated = 0;
    is_ip_offered = 0;
    dhcp_discover();
    break;
  }
}