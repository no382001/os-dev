#include "icmp.h"
#include "drivers/serial.h"
#include "libc/heap.h"
#include "libc/mem.h"
#include "network.h"

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
  packet->id = htons(id);
  packet->sequence = htons(sequence);

  if (data && data_len > 0) {
    memcpy(packet->data, data, data_len);
  }

  packet->checksum = _ip_calculate_checksum((ip_packet_t *)packet, icmp_size);

  _ip_send_packet(dst_ip, packet, icmp_size, PROTOCOL_ICMP);

  kfree(packet);
}

void icmp_send_echo_request(uint8_t *dst_ip, uint16_t id, uint16_t sequence,
                            void *data, uint32_t data_len) {
  icmp_send_packet(dst_ip, ICMP_ECHO_REQUEST, 0, id, sequence, data, data_len);
}