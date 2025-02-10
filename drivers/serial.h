#pragma once

#define SERIAL_COM1 0x3F8

void serial_init();
void serial_puts(const char *str);
void serial_debug_impl(char *file, int line, char *message);

#define serial_debug(message) serial_debug_impl(__FILE__, __LINE__, message);
