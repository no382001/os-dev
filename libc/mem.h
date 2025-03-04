#pragma once

#include "types.h"

void *memcpy(void *dest, const void *src, size_t num_bytes);
void memset(void *dest, int val, uint32_t len);
void *memmove(void *dest, const void *src, size_t num_bytes);