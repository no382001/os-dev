#include "bits.h"

void hexdump(const char *buffer, int size, int colsize) {
  if (colsize <= 0 || colsize > 16) {
    kernel_printf("hexdump: invalid colsize: %d. Must be between 1 and 16.", colsize);
    return;
  }

  char ascii[17];
  ascii[colsize] = '\0';

  for (int i = 0; i < size; i++) {
    if (i % colsize == 0) {
      if (i != 0) {
        kernel_printf(" | %s |", ascii);
      }
      kernel_printf("\n%x |  ", i);
    }

    kernel_printf("%x ", (unsigned char)buffer[i]);

    ascii[i % colsize] =
        (buffer[i] >= 32 && buffer[i] <= 126) ? buffer[i] : ' ';
  }

  // fill last line with spaces if not aligned to `colsize`
  int remaining = size % colsize;
  if (remaining > 0) {
    for (int i = remaining; i < colsize; i++) {
      kernel_printf("     %s", (i % colsize == colsize - 1) ? " " : "");
      ascii[i] = ' ';
    }
    kernel_printf(" | %s |", ascii);
  }

  kernel_printf("\n");
}
