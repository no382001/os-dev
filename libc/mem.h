#pragma once

#include "types.h"

void *memcpy(void *dest, const void *src, size_t num_bytes);
void memset(void *dest, int val, uint32_t len);
void *memmove(void *dest, const void *src, size_t num_bytes);
uint32_t memcmp(uint8_t *data1, uint8_t *data2, uint32_t n);