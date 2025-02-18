#pragma once

#define VIDEO_ADDRESS ((char *)0xb8000)
#define MAX_ROWS 25
#define MAX_COLS 80

#define BLUE_ON_BLACK 0x01
#define GREEN_ON_BLACK 0x02
#define CYAN_ON_BLACK 0x03
#define RED_ON_BLACK 0x04
#define MAGENTA_ON_BLACK 0x05
#define BROWN_ON_BLACK 0x06
#define LIGHT_GRAY_ON_BLACK 0x07
#define DARK_GRAY_ON_BLACK 0x08
#define LIGHT_BLUE_ON_BLACK 0x09
#define LIGHT_GREEN_ON_BLACK 0x0A
#define LIGHT_CYAN_ON_BLACK 0x0B
#define LIGHT_RED_ON_BLACK 0x0C
#define LIGHT_MAGENTA_ON_BLACK 0x0D
#define YELLOW_ON_BLACK 0x0E
#define WHITE_ON_BLACK 0x0F

// screen device I/O ports
#define REG_SCREEN_CTRL 0x3D4
#define REG_SCREEN_DATA 0x3D5

void kernel_clear_screen();
void kernel_print_c_at(char character, int col, int row, char attribute_byte);
void kernel_print_string_at(char *message, int col, int row);
void kernel_putc(char c);
void kernel_puts(char *message);
void kernel_puth(int value);
void kernel_put_backspace();
void kernel_print_set_attribute(char a);

void kernel_printf(const char *fmt, ...);
void kernel_aprintf(char attribute, const char *fmt, ...);