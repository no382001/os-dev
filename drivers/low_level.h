#pragma once
#include "libc/types.h"

uint8_t port_byte_in(uint16_t port);
void port_byte_out(uint16_t port, uint8_t data);
uint16_t port_word_in(uint16_t port);
void port_word_out(uint16_t port, uint16_t data);
uint32_t port_long_in(uint16_t port);
void port_long_out(uint16_t port, uint32_t data);

void ata_read_sector(uint32_t lba, uint8_t *buffer);
void ata_write_sector(uint32_t lba, uint8_t *buffer);

uint32_t crc32c(uint32_t crc, const unsigned char *buf, size_t len);