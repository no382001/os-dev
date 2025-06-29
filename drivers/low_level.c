#include "low_level.h"
#include "bits.h"

uint8_t port_byte_in(uint16_t port) {
  unsigned char result;
  __asm__("in %%dx , %%al " : "=a"(result) : "d"(port));
  return result;
}

void port_byte_out(uint16_t port, uint8_t data) {
  __asm__("out %%al, %%dx" : : "a"(data), "d"(port));
}

uint16_t port_word_in(uint16_t port) {
  uint16_t result;
  __asm__("in %%dx, %%ax" : "=a"(result) : "d"(port));
  return result;
}

void port_word_out(uint16_t port, uint16_t data) {
  __asm__("out %%ax, %%dx" : : "a"(data), "d"(port));
}

uint32_t port_long_in(uint16_t port) {
  uint32_t result;
  __asm__("in %%dx, %%eax" : "=a"(result) : "d"(port));
  return result;
}

void port_long_out(uint16_t port, uint32_t data) {
  __asm__("out %%eax, %%dx" : : "a"(data), "d"(port));
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

void ata_write_sector(uint32_t lba, uint8_t *buffer) {
  port_byte_out(0x1F6, (lba >> 24) | 0xE0);
  port_byte_out(0x1F2, 1); // one sector
  port_byte_out(0x1F3, (uint8_t)lba);
  port_byte_out(0x1F4, (uint8_t)(lba >> 8));
  port_byte_out(0x1F5, (uint8_t)(lba >> 16));
  port_byte_out(0x1F7, 0x30); // WRITE SECTOR

  int timeout = 100000;
  while (timeout--) {
    uint8_t status = port_byte_in(0x1F7);
    if (status & 0x01) {
      serial_debug("disk write error at LBA %d\n", lba);
      return;
    }
    if (status & 0x08) // data ready
      break;
  }
  if (timeout <= 0) {
    serial_debug("ERROR: ata_write_sector() - timeout waiting for drive!\n");
    return;
  }

  for (int i = 0; i < 512 / 2; i++) {
    port_word_out(0x1F0, ((uint16_t *)buffer)[i]);
  }

  timeout = 100000;
  while (timeout--) {
    uint8_t status = port_byte_in(0x1F7);
    if (!(status & 0x80)) // BSY bit clear
      break;
  }
}