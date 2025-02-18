#include "low_level.h"
#include "bits.h"

unsigned char port_byte_in(unsigned short port) {
  unsigned char result;
  __asm__("in %%dx , %%al " : "=a"(result) : "d"(port));
  return result;
}

void port_byte_out(unsigned short port, unsigned char data) {
  __asm__("out %%al, %%dx" : : "a"(data), "d"(port));
}

unsigned short port_word_in(unsigned short port) {
  unsigned short result;
  __asm__("in %%dx, %%ax" : "=a"(result) : "d"(port));
  return result;
}

void port_word_out(unsigned short port, unsigned short data) {
  __asm__("out %%ax, %%dx" : : "a"(data), "d"(port));
}

void ata_read_sector(uint32_t lba, uint8_t *buffer) {

  port_byte_out(0x1F6, (lba >> 24) | 0xE0);
  port_byte_out(0x1F2, 1); // one sector
  port_byte_out(0x1F3, (uint8_t)lba);
  port_byte_out(0x1F4, (uint8_t)(lba >> 8));
  port_byte_out(0x1F5, (uint8_t)(lba >> 16));
  port_byte_out(0x1F7, 0x20); // READ SECTOR

  // serial_debug(" trying to read sector %d", lba);
  int timeout = 100000;
  while (timeout--) {
    uint8_t status = port_byte_in(0x1F7);
    if (status & 0x01) {
      serial_debug("disk read error at LBA %d\n", lba);
      return;
    }
    if (status & 0x08) // data ready
      break;
  }
  if (timeout <= 0) {
    serial_debug("ERROR: ata_read_sector() - timeout waiting for drive!\n");
    return;
  }

  // serial_debug(" found sector %d", lba);
  for (int i = 0; i < 512 / 2; i++) { // we read 2 bytes
    ((uint16_t *)buffer)[i] = port_word_in(0x1F0);
  }
}