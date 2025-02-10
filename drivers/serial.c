#include "serial.h"
#include "kernel/utils.h"
#include "low_level.h"

void serial_init() {
  port_byte_out(SERIAL_COM1 + 1, 0x00); // disable interrupts
  port_byte_out(SERIAL_COM1 + 3, 0x80); // enable DLAB (set baud rate divisor)
  port_byte_out(SERIAL_COM1 + 0, 0x03); // set divisor to 3 (38400 baud)
  port_byte_out(SERIAL_COM1 + 1, 0x00); // high byte of divisor
  port_byte_out(SERIAL_COM1 + 3, 0x03); // 8 bits, no parity, 1 stop bit
  port_byte_out(SERIAL_COM1 + 2,
                0xC7); // enable FIFO, clear it, 14-byte threshold
  port_byte_out(SERIAL_COM1 + 4, 0x0B); // enable IRQs, RTS/DSR set
}

static int serial_is_transmit_empty() {
  return port_byte_in(SERIAL_COM1 + 5) & 0x20;
}

static void serial_write(char c) {
  while (!serial_is_transmit_empty())
    ;
  port_byte_out(SERIAL_COM1, c);
}

void serial_puts(const char *str) {
  while (*str) {
    serial_write(*str++);
  }
}

void serial_debug_impl(char *file, int line, char *message) {
  serial_puts(file);
  serial_puts(":");
  char c[15];
  int_to_ascii(line, c);
  serial_puts(c);
  serial_puts(" ");
  serial_puts(message);
}