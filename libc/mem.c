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
