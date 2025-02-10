#include "serial.h"
#include "bits.h"
#include "libc/string.h"
#include "low_level.h"

static char serial_buffer[256];

void user_input(char *input);
static void serial_irq_handler(registers_t *regs) {
  (void)regs;

  if (!(port_byte_in(SERIAL_COM1 + 5) & 0x01))
    return;

  char c = port_byte_in(SERIAL_COM1);

  if (c == '\r' || c == '\n') {
    user_input(serial_buffer);
    serial_buffer[0] = '\0';
  } else {
    int len = strlen(serial_buffer);
    if (len < (int)sizeof(serial_buffer) - 1) {
      serial_buffer[len] = c;
      serial_buffer[len + 1] = '\0';
    }
  }
}

void serial_init() {
  port_byte_out(SERIAL_COM1 + 1, 0x00); // disable interrupts
  port_byte_out(SERIAL_COM1 + 3, 0x80); // enable DLAB (set baud rate divisor)
  port_byte_out(SERIAL_COM1 + 0, 0x03); // set divisor to 3 (38400 baud)
  port_byte_out(SERIAL_COM1 + 1, 0x00); // high byte of divisor
  port_byte_out(SERIAL_COM1 + 3, 0x03); // 8 bits, no parity, 1 stop bit
  port_byte_out(SERIAL_COM1 + 2,
                0xC7); // enable FIFO, clear it, 14-byte threshold
  port_byte_out(SERIAL_COM1 + 4, 0x0B); // enable IRQs, RTS/DSR set

  port_byte_out(SERIAL_COM1 + 1,
                0x01); // received data available interrupt (bit 0 in the IER)
  register_interrupt_handler(IRQ4, serial_irq_handler);
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