#include "bits.h"

#undef serial_debug
#define serial_debug(...)

void *memcpy(void *dest, const void *src, size_t num_bytes) {
  uint8_t *d = (uint8_t *)dest;
  const uint8_t *s = (const uint8_t *)src;

  while (((uintptr_t)d % 4) && num_bytes) {
    *d++ = *s++;
    num_bytes--;
  }

  while (num_bytes >= 4) {
    *((uint32_t *)d) = *((uint32_t *)s);
    d += 4;
    s += 4;
    num_bytes -= 4;
  }

  while (num_bytes--) {
    *d++ = *s++;
  }

  return dest;
}

void memset(void *dest, int val, uint32_t len) {
  serial_debug("memset called on %x, value=%d, size=%d", dest, val, len);
  uint8_t *temp = (uint8_t *)dest;
  for (; len != 0; len--) {
    *temp++ = val;
  }
}

void *memmove(void *dest, const void *src, size_t num_bytes) {
  uint8_t *d = (uint8_t *)dest;
  const uint8_t *s = (const uint8_t *)src;

  // If destination and source overlap and destination is ahead of source,
  // we need to copy backward to avoid overwriting source data that we still
  // need
  if ((d > s) && (d < s + num_bytes)) {
    // Start from the end and work backwards
    d += num_bytes - 1;
    s += num_bytes - 1;

    // Handle byte-by-byte first for small copies
    while (num_bytes--) {
      *d-- = *s--;
    }
  } else {
    // No overlap or dest is before src, so we can use optimized forward copy

    // Align destination to 4-byte boundary for faster copying
    while (((uintptr_t)d % 4) && num_bytes) {
      *d++ = *s++;
      num_bytes--;
    }

    // Copy 4 bytes at a time for speed
    while (num_bytes >= 4) {
      *((uint32_t *)d) = *((uint32_t *)s);
      d += 4;
      s += 4;
      num_bytes -= 4;
    }

    // Handle remaining bytes
    while (num_bytes--) {
      *d++ = *s++;
    }
  }

  return dest;
}

uint32_t memcmp(uint8_t *data1, uint8_t *data2, uint32_t n) {
  while (n--) {
    if (*data1 != *data2)
      return 1;
    data1++;
    data2++;
  }
  return 0;
}