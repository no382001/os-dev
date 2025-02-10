#pragma once

#include "types.h"

void memcpy(char *source, char *dest, int no_bytes);
void memset(uint8_t *dest, uint8_t val, uint32_t len);
size_t kernel_malloc(size_t size, int align, size_t *phys_addr);