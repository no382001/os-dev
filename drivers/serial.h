#pragma once
#include "libc/types.h"
#include <stdarg.h> // va_list and friends
#define SERIAL_COM1 0x3F8

void serial_init();
void serial_puts(const char *str);
void serial_put_hex(uint32_t num);
void serial_debug_impl(char *file, int line, char *message);

// dont use this directly
void serial_printf(char *file, int line, const char *fmt, ...);

#define serial_debug(...) serial_printf(__FILE__, __LINE__, __VA_ARGS__);
// format to the serial
#define serial_printff(...) serial_printf(0, 0, __VA_ARGS__);