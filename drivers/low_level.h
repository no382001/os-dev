#pragma once
#include "libc/types.h"

unsigned char port_byte_in(unsigned short port);
void port_byte_out(unsigned short port, unsigned char data);
unsigned short port_word_in(unsigned short port);
void port_word_out(unsigned short port, unsigned short data);

void ata_read_sector(uint32_t lba, uint8_t *buffer);