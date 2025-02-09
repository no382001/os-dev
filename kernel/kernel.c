#include "cpu/idt.h"
#include "cpu/isr.h"
#include "drivers/screen.h"
#include "utils.h"

#include "drivers/low_level.h"

#define SERIAL_COM1 0x3F8

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

int serial_is_transmit_empty() { return port_byte_in(SERIAL_COM1 + 5) & 0x20; }

void serial_write(char c) {
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

#define serial_debug(message) serial_debug_impl(__FILE__, __LINE__, message);

void main(void) {
  kernel_clear_screen();
  serial_init();
  isr_install();

  kernel_puts("hello kernel!\n");
  serial_debug("hello serial!\n");

  __asm__ __volatile__("int $2");
  __asm__ __volatile__("int $3");
  kernel_puth(255);
}
