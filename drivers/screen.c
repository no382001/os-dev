#include "bits.h"

static char current_attribute = WHITE_ON_BLACK;

void kernel_print_set_attribute(char a) { current_attribute = a; }

static int get_screen_offset(int col, int row) {
  return 2 * (row * MAX_COLS + col);
}

static int handle_scrolling(int offset) {
  if (offset < MAX_ROWS * MAX_COLS * 2) {
    return offset;
  }

  memcpy((char *)VIDEO_ADDRESS, (char *)(VIDEO_ADDRESS + MAX_COLS * 2),
         (MAX_ROWS - 1) * MAX_COLS * 2);

  for (int j = 0; j < MAX_COLS; j++) {
    *((char *)VIDEO_ADDRESS + (MAX_ROWS - 1) * MAX_COLS * 2 + j * 2) = ' ';
    *((char *)VIDEO_ADDRESS + (MAX_ROWS - 1) * MAX_COLS * 2 + j * 2 + 1) =
        WHITE_ON_BLACK;
  }

  return get_screen_offset(0, MAX_ROWS - 1);
}

static void set_cursor(int offset) {
  offset /= 2; // byte offset to character cell offset
  port_byte_out(REG_SCREEN_CTRL, 14);
  port_byte_out(REG_SCREEN_DATA, (unsigned char)(offset >> 8)); // hi
  port_byte_out(REG_SCREEN_CTRL, 15);
  port_byte_out(REG_SCREEN_DATA, (unsigned char)(offset & 0xFF)); // lo
}

static int get_cursor() {
  port_byte_out(REG_SCREEN_CTRL, 14);
  int offset = port_byte_in(REG_SCREEN_DATA) << 8;
  port_byte_out(REG_SCREEN_CTRL, 15);
  offset += port_byte_in(REG_SCREEN_DATA);
  return offset * 2;
}

void kernel_print_c_at(char character, int col, int row, char attribute_byte) {
  unsigned char *vidmem = (unsigned char *)VIDEO_ADDRESS;

  if (!attribute_byte) {
    attribute_byte = current_attribute;
  }

  int offset;
  if (col >= 0 && row >= 0) {
    offset = get_screen_offset(col, row);
  } else {
    offset = get_cursor();
  }

  if (character == '\n') {
    int rows = offset / (2 * MAX_COLS);
    offset = get_screen_offset(0, rows + 1);
  } else {
    vidmem[offset] = character;
    vidmem[offset + 1] = attribute_byte;
    offset += 2;
  }

  offset = handle_scrolling(offset);
  set_cursor(offset);
}

void kernel_clear_screen() {
  int screen_size = MAX_COLS * MAX_ROWS;
  int i;
  char *screen = VIDEO_ADDRESS;

  for (i = 0; i < screen_size; i++) {
    screen[i * 2] = ' ';
    screen[i * 2 + 1] = WHITE_ON_BLACK;
  }
  set_cursor(get_screen_offset(0, 0));
}

void kernel_puth(int value) {
  static char hex_chars[] = "0123456789ABCDEF";
  kernel_print_c_at('0', -1, -1, WHITE_ON_BLACK);
  kernel_print_c_at('x', -1, -1, WHITE_ON_BLACK);

  for (int i = 7; i >= 0; i--) {
    unsigned char nibble = (value >> (i * 4)) & 0xF;
    kernel_print_c_at(hex_chars[nibble], -1, -1, WHITE_ON_BLACK);
  }
}

void kernel_print_string_at(char *message, int col, int row) {
  int offset;

  if (col >= 0 && row >= 0) {
    offset = get_screen_offset(col, row);
    set_cursor(offset);
  } else {
    offset = get_cursor();
  }

  int i = 0;
  while (message[i] != 0) {
    kernel_print_c_at(message[i], -1, -1, WHITE_ON_BLACK);
    i++;
  }
}

void kernel_puts(char *message) { kernel_print_string_at(message, -1, -1); }

void kernel_putc(char c) { kernel_print_c_at(c, -1, -1, 0); }

int get_offset_row(int offset) { return offset / (2 * MAX_COLS); }
int get_offset_col(int offset) {
  return (offset - (get_offset_row(offset) * 2 * MAX_COLS)) / 2;
}

void kernel_put_backspace() {
  int offset = get_cursor() - 2;
  int row = get_offset_row(offset);
  int col = get_offset_col(offset);
  kernel_print_c_at(' ', col, row, WHITE_ON_BLACK);
  set_cursor(offset);
}

void kernel_printf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  _vprintf(kernel_putc, fmt, args);
  va_end(args);
}

void kernel_aprintf(char attribute, const char *fmt, ...) {
  kernel_print_set_attribute(attribute);
  va_list args;
  va_start(args, fmt);
  kernel_printf(fmt, args);
  va_end(args);
  kernel_print_set_attribute(WHITE_ON_BLACK);
}