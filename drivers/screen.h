#pragma once

#define VIDEO_ADDRESS ((char *)0xb8000)
#define MAX_ROWS 25
#define MAX_COLS 80

#define WHITE_ON_BLACK 0x0f

// screen device I/O ports
#define REG_SCREEN_CTRL 0x3D4
#define REG_SCREEN_DATA 0x3D5

void kernel_clear_screen();
void kernel_print_c_at(char character, int col, int row, char attribute_byte);
void kernel_print_string_at(char *message, int col, int row);
void kernel_putc(char c);
void kernel_puts(char *message);
void kernel_puth(int value);