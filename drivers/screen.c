#include "bits.h"

/*******************************
 * text mode 0x3
 */

int is_mode_03h() {
  unsigned char mode;
  __asm__ volatile("movb 0x449, %0" : "=r"(mode));
  return (mode == 0x3);
}

static char current_attribute = WHITE_ON_BLACK;

void kernel_print_set_attribute(char a) {
  if (!is_mode_03h()) {
    return;
  }
  current_attribute = a;
}

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

static void kernel_print_c_at(char character, int col, int row,
                              char attribute_byte) {
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
  if (!is_mode_03h()) {
    return;
  }

  int screen_size = MAX_COLS * MAX_ROWS;
  int i;
  char *screen = VIDEO_ADDRESS;

  for (i = 0; i < screen_size; i++) {
    screen[i * 2] = ' ';
    screen[i * 2 + 1] = WHITE_ON_BLACK;
  }
  set_cursor(get_screen_offset(0, 0));
}

static void kernel_print_string_at(char *message, int col, int row) {
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

void kernel_puts(char *message) {
  if (!is_mode_03h()) {
    return;
  }
  kernel_print_string_at(message, -1, -1);
}

void kernel_putc(char c) {
  if (!is_mode_03h()) {
    return;
  }
  kernel_print_c_at(c, -1, -1, 0);
}

int get_offset_row(int offset) { return offset / (2 * MAX_COLS); }
int get_offset_col(int offset) {
  return (offset - (get_offset_row(offset) * 2 * MAX_COLS)) / 2;
}

void kernel_put_backspace() {
  if (!is_mode_03h()) {
    return;
  }

  int offset = get_cursor() - 2;
  int row = get_offset_row(offset);
  int col = get_offset_col(offset);
  kernel_print_c_at(' ', col, row, WHITE_ON_BLACK);
  set_cursor(offset);
}

void kernel_printf(const char *fmt, ...) {
  if (!is_mode_03h()) {
    return;
  }

  va_list args;
  va_start(args, fmt);
  _vprintf(kernel_putc, fmt, args);
  va_end(args);
}

/*******************************
 * VGA mode 0x12
 * https://web.archive.org/web/20121007073509/http://dimensionalrift.homelinux.net/combuster/mos3/?p=viewsource&file=/modules/vga_io.bas
 * https://en.wikipedia.org/wiki/Planar_(computer_graphics)
 */

static void _vga_unlock_crt_regs() {
  WRITE_CRTC(0x11, port_byte_in(VGA_CRTC_DATA) & 0x7F);
}

void _vga_enable_display() {
  port_byte_in(VGA_INPUT_STATUS);
  port_byte_out(VGA_ATTRIBUTE_INDEX, 0x20);
}

void set_vga_mode12() {
  asm volatile("cli");

  _vga_unlock_crt_regs(); // unlock VGA registers for modification

  WRITE_MISC(0xE3);

  // VGA sequencer registers (memory configuration)
  WRITE_SEQUENCER(0x01, 0x01); // clocking Mode
  WRITE_SEQUENCER(0x02, 0x08); // enable plane 2 (planar mode)
  WRITE_SEQUENCER(0x04, 0x06); // memory mode: enable graphics

  // VGA CRT controller registers - Set 640x480 resolution
  WRITE_CRTC(0x00, 0x5F); // horizontal total
  WRITE_CRTC(0x01, 0x4F); // horizontal display end
  WRITE_CRTC(0x02, 0x50); // start horizontal blanking
  WRITE_CRTC(0x03, 0x82); // end horizontal blanking
  WRITE_CRTC(0x04, 0x54); // start horizontal retrace
  WRITE_CRTC(0x05, 0x80); // end horizontal retrace
  WRITE_CRTC(0x06, 0x0B); // vertical total
  WRITE_CRTC(0x07, 0x3E); // overflow register
  WRITE_CRTC(0x08, 0x00); // preset row scan
  WRITE_CRTC(0x09, 0x40); // maximum scan line
  WRITE_CRTC(0x10, 0xEA); // vertical retrace start
  WRITE_CRTC(0x11, 0x0C); // vertical retrace end
  WRITE_CRTC(0x12, 0xDF); // vertical display end
  WRITE_CRTC(0x13, 0x28); // offset (bytes per scan line / 2)
  WRITE_CRTC(0x14, 0x00); // underline Location
  WRITE_CRTC(0x15, 0xE7); // vertical Blank Start
  WRITE_CRTC(0x16, 0x04); // vertical Blank End
  WRITE_CRTC(0x17, 0xE3); // mode Control

  // VGA graphics controller registers (handling bit planes)
  WRITE_GRAPHICS(0x00, 0x00); // set/reset register
  WRITE_GRAPHICS(0x01, 0x00); // enable set/reset
  WRITE_GRAPHICS(0x02, 0x00); // color compare
  WRITE_GRAPHICS(0x03, 0x00); // data rotate
  WRITE_GRAPHICS(0x04, 0x03); // read map select (planes)
  WRITE_GRAPHICS(0x05, 0x00); // mode register
  WRITE_GRAPHICS(0x06, 0x05); // miscellaneous register
  WRITE_GRAPHICS(0x07, 0x0F); // color don't care
  WRITE_GRAPHICS(0x08, 0xFF); // bit mask register

  // VGA attribute controller registers (color palette and mode control)
  WRITE_ATTRIBUTE(0x10, 0x01); // mode control
  WRITE_ATTRIBUTE(0x11, 0x00); // overscan color
  WRITE_ATTRIBUTE(0x12, 0x0F); // color plane enable
  WRITE_ATTRIBUTE(0x13, 0x00); // horizontal pixel panning
  WRITE_ATTRIBUTE(0x14, 0x00); // color select

  _vga_enable_display();

  asm volatile("sti");
}

#define VGA_BUFFER_SIZE ((VGA_WIDTH / 8) * VGA_HEIGHT)

uint8_t *vga_bb = 0;

void vga_put_pixel(int x, int y, unsigned char color) {
  if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT)
    return;

  // calculate the bit mask to select the correct pixel within a byte
  // since each byte in vga memory holds 8 pixels, we shift a some bits to the
  // right
  uint8_t mask = 0x80 >> (x % 8);
  // compute the memory offset for the pixel
  // since each row consists of (640 / 8) bytes (80 bytes per row),
  // we multiply the row index (y) by 80 and add the byte index (x / 8)
  int offset = (y * (VGA_WIDTH / 8)) + (x / 8);

  for (int plane = 0; plane < 4; plane++) {
    uint8_t *plane_buffer = &vga_bb[plane * VGA_BUFFER_SIZE];
    if (color & (1 << plane)) {
      plane_buffer[offset] |= mask;
    } else {
      plane_buffer[offset] &= ~mask; // clear, or maybe dont do anything here?
    }
  }
}

void vga_clear_screen(unsigned char color) {
  // calculate the total number of bytes in vga memory for one plane
  int size = (VGA_WIDTH / 8) * VGA_HEIGHT;

  uint32_t fill_value;
  uint8_t *ptr;

  // loop through each of the 4 bit planes to clear them one by one
  for (int plane = 0; plane < 4; plane++) {
    // select the current vga memory plane
    port_byte_out(VGA_SEQUENCER_INDEX, 0x02); // select the map mask register
    port_byte_out(VGA_SEQUENCER_DATA,
                  1 << plane); // enable only the current plane

    // determine the fill value for the plane
    // if the bit corresponding to this plane in the color parameter is set,
    // fill the plane with 0xff (white), otherwise fill it with 0x00 (black)
    uint8_t byte_fill = (color & (1 << plane)) ? 0xFF : 0x00;

    // construct a 32-bit fill value by repeating the byte fill pattern
    fill_value =
        (byte_fill << 24) | (byte_fill << 16) | (byte_fill << 8) | byte_fill;

    ptr = vga_bb;
    for (int i = 0; i < size / 4; i++) {
      *((uint32_t *)ptr) = fill_value;
      ptr += 4;
    }
  }
}

void vga_swap_buffers() {
  uint8_t *vga = (uint8_t *)0xA0000;
  int size = VGA_BUFFER_SIZE;

  for (int plane = 0; plane < 4; plane++) {
    port_byte_out(VGA_SEQUENCER_INDEX, 0x02); // select the map mask register
    port_byte_out(VGA_SEQUENCER_DATA,
                  1 << plane); // enable only the current plane

    memcpy((void *)vga, (void *)&vga_bb[plane * VGA_BUFFER_SIZE], size);
  }
}

void vga_draw_filled_rect(int x, int y, int width, int height, uint8_t color) {
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      vga_put_pixel(x + i, y + j, color);
    }
  }
}

void vga_draw_rect(int x, int y, int width, int height, uint8_t color) {
  for (int i = 0; i < width; i++) {
    vga_put_pixel(x + i, y, color);
    vga_put_pixel(x + i, y + height - 1, color);
  }
  for (int j = 0; j < height; j++) {
    vga_put_pixel(x, y + j, color);
    vga_put_pixel(x + width - 1, y + j, color);
  }
}

void vga_clear_rect(int x, int y, int width, int height) {
  vga_draw_filled_rect(x, y, width, height, 0x00);
}
