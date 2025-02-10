#include "mem.h"

void memcpy(char *source, char *dest, int no_bytes) {
  int i;
  for (i = 0; i < no_bytes; i++) {
    *(dest + i) = *(source + i);
  }
}

void memset(u8 *dest, u8 val, u32 len) {
  u8 *temp = (u8 *)dest;
  for (; len != 0; len--)
    *temp++ = val;
}