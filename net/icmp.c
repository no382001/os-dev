#include "icmp.h"
#include "drivers/serial.h"
#include "libc/heap.h"
#include "libc/mem.h"
#include "network.h"

static uint16_t icmp_calculate_checksum(void *buffer, uint16_t size) {
  uint32_t sum = 0;
  uint16_t *ptr = (uint16_t *)buffer;

  // sum all 16-bit words
  for (int i = 0; i < size / 2; i++) {
    sum += ntohs(ptr[i]);
  }

  // handle odd byte if present
  if (size % 2) {
    sum += ((uint8_t *)buffer)[size - 1];
  }

  // add carry bits and fold to 16 bits
  while (sum >> 16) {
    sum = (sum & 0xFFFF) + (sum >> 16);
  }

  // return ones complement
  return htons(~sum);
}

void icmp_send_packet(uint8_t *dst_ip, uint8_t type, uint8_t code, uint16_t id,
                      uint16_t sequence, void *data, uint32_t data_len) {
  serial_debug("sending icmp packet %d to %d.%d.%d.%d", type, dst_ip[0],
               dst_ip[1], dst_ip[2], dst_ip[3]);

  uint32_t icmp_size = sizeof(icmp_packet_t) + data_len;

  icmp_packet_t *packet = (icmp_packet_t *)kmalloc(icmp_size);
  if (!packet) {
    serial_debug("failed to allocate memory for ICMP packet");
    return;
  }

  packet->type = type;
  packet->code = code;
  packet->checksum = 0;
  packet->id = id;
  packet->sequence = sequence;

  if (data && data_len > 0) {
    memcpy(packet->data, data, data_len);
  }

  packet->checksum = icmp_calculate_checksum(packet, icmp_size);

  _ip_send_packet(dst_ip, packet, icmp_size, PROTOCOL_ICMP);

  kfree(packet);
}

void icmp_send_echo_request(uint8_t *dst_ip, uint16_t id, uint16_t sequence,
                            void *data, uint32_t data_len) {
  icmp_send_packet(dst_ip, ICMP_ECHO_REQUEST, 0, id, sequence, data, data_len);
}

void icmp_handle_packet(icmp_packet_t *packet, uint16_t length,
                        uint8_t *src_ip) {

  uint16_t received_checksum = packet->checksum;
  packet->checksum = 0;
  uint16_t calculated_checksum =
      icmp_calculate_checksum((ip_packet_t *)packet, length);
  packet->checksum = received_checksum;

  if (received_checksum != calculated_checksum) {
    serial_debug("icmp checksum mismatch: received=%x, calculated=%x",
                 received_checksum, calculated_checksum);
    return;
  }

  switch (packet->type) {
  case ICMP_ECHO_REQUEST:
    serial_debug("received echo request from %d.%d.%d.%d (id=%d, seq=%d)",
                 src_ip[0], src_ip[1], src_ip[2], src_ip[3], ntohs(packet->id),
                 ntohs(packet->sequence));

    icmp_send_packet(src_ip, ICMP_ECHO_REPLY, 0, packet->id, packet->sequence,
                     packet->data, length - sizeof(icmp_packet_t));
    break;

  case ICMP_ECHO_REPLY:
    serial_debug("received echo reply from %d.%d.%d.%d (id=%d, seq=%d)",
                 src_ip[0], src_ip[1], src_ip[2], src_ip[3], ntohs(packet->id),
                 ntohs(packet->sequence));

    // calculate round-trip time,
    // update ping statistics, or notify a waiting ping process
    // that a reply has arrived, not now....

    break;

  case ICMP_DEST_UNREACHABLE:
    serial_debug("destination unreachable message from %d.%d.%d.%d, code=%d",
                 src_ip[0], src_ip[1], src_ip[2], src_ip[3], packet->code);
    break;

  case ICMP_TIME_EXCEEDED:
    serial_debug("time exceeded message from %d.%d.%d.%d, code=%d", src_ip[0],
                 src_ip[1], src_ip[2], src_ip[3], packet->code);
    break;

  default:
    serial_debug("unhandled ICMP type: %d", packet->type);
    break;
  }
}