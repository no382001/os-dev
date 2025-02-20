#include "bits.h"

void hexdump(const char *buffer, int size, int colsize) {
  if (colsize <= 0 || colsize > 16) {
    kernel_printf("hexdump: invalid colsize: %d. Must be between 1 and 16.\n",
                  colsize);
    return;
  }

  char ascii[17];
  ascii[colsize] = '\0';

  for (int i = 0; i < size; i++) {
    if (i % colsize == 0) {
      if (i != 0) {
        kernel_printf(" | %s |\n", ascii);
      }
      kernel_printf("%08x | ", (uint32_t)(buffer + i));
    }

    kernel_printf("%02x ", (unsigned char)buffer[i]);

    ascii[i % colsize] =
        (buffer[i] >= 32 && buffer[i] <= 126) ? buffer[i] : '.';

    if (i == size - 1) {
      int remaining = (i + 1) % colsize;
      if (remaining > 0) {
        for (int j = remaining; j < colsize; j++) {
          kernel_printf("   ");
        }
      }
      kernel_printf(" | %s |\n", ascii);
    }
  }
}
