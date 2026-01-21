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
  cli();

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

  sti();
}

uint8_t *vga_bb = 0;

void init_vga12h() {
  vga_bb = (uint8_t *)kmalloc(VGA_BUFFER_SIZE * 4);
  assert(vga_bb);
}

#define VGA_BUFFER_SIZE ((VGA_WIDTH / 8) * VGA_HEIGHT)

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

void init_font(font_t *font, bdf_font_t *bfd) {
  assert(bfd);
  font->color = WHITE_ON_BLACK;
  font->scale_x = 1;
  font->scale_y = 1;
  font->bfd = bfd;
}

void draw_bdf_char(int x, int y, char c, font_t *font) {
  assert(font);
  if (c < 0)
    return;

  glyph_t glyph = font->bfd->glyphs[(uint8_t)c];

  int glyph_width = glyph.width;
  int glyph_height = glyph.height;
  int bytes_per_row = (glyph_width + 7) / 8;

  int adjusted_y =
      y + (font->bfd->glyphs['A'].height - glyph.height) * font->scale_y;

  for (int row = 0; row < glyph_height; row++) {
    for (int col = 0; col < glyph_width; col++) {
      int byte_index = col / 8;
      int bit_position = 7 - (col % 8);

      if (glyph.bitmap[row * bytes_per_row + byte_index] &
          (1 << bit_position)) {
        for (int sy = 0; sy < font->scale_y; sy++) {
          for (int sx = 0; sx < font->scale_x; sx++) {
            vga_put_pixel(x + col * font->scale_x + sx,
                          adjusted_y + row * font->scale_y + sy, font->color);
          }
        }
      }
    }
  }
}

void draw_bdf_string(int x, int y, const char *str, font_t *font) {
  int offset_x = 0;

  while (*str) {
    if (*str == '\n') {
      y += font->bfd->glyphs['A'].height * font->scale_y;
      offset_x = 0;
    } else if (*str == ' ') {
      x += font->bfd->glyphs['A'].width;
    } else {
      draw_bdf_char(x + offset_x, y, *str, font);
      offset_x += (font->bfd->glyphs[(uint8_t)*str].width + 1) * font->scale_x;
    }
    str++;
  }
}

/*******************************
 * VGA mode 0x13 (320x200, 256 colors)
 */

static uint8_t *vga13_bb = NULL;

void set_vga_mode13() {
  cli();

  _vga_unlock_crt_regs();

  WRITE_MISC(0x63);

  WRITE_SEQUENCER(0x01, 0x01); // clocking mode
  WRITE_SEQUENCER(0x02, 0x0F); // enable all planes
  WRITE_SEQUENCER(0x04, 0x0E); // memory mode: chain-4

  WRITE_CRTC(0x00, 0x5F); // horizontal total
  WRITE_CRTC(0x01, 0x4F); // horizontal display end
  WRITE_CRTC(0x02, 0x50); // start horizontal blanking
  WRITE_CRTC(0x03, 0x82); // end horizontal blanking
  WRITE_CRTC(0x04, 0x54); // start horizontal retrace
  WRITE_CRTC(0x05, 0x80); // end horizontal retrace
  WRITE_CRTC(0x06, 0xBF); // vertical total
  WRITE_CRTC(0x07, 0x1F); // overflow
  WRITE_CRTC(0x08, 0x00); // preset row scan
  WRITE_CRTC(0x09, 0x41); // max scan line (double scan)
  WRITE_CRTC(0x10, 0x9C); // vertical retrace start
  WRITE_CRTC(0x11, 0x0E); // vertical retrace end
  WRITE_CRTC(0x12, 0x8F); // vertical display end
  WRITE_CRTC(0x13, 0x28); // offset
  WRITE_CRTC(0x14, 0x40); // underline location (enable dword mode)
  WRITE_CRTC(0x15, 0x96); // vertical blank start
  WRITE_CRTC(0x16, 0xB9); // vertical blank end
  WRITE_CRTC(0x17, 0xA3); // mode control

  WRITE_GRAPHICS(0x05, 0x40); // 256-color mode
  WRITE_GRAPHICS(0x06, 0x05); // memory map

  port_byte_in(VGA_INPUT_STATUS); // reset flip-flop
  for (int i = 0; i < 16; i++) {
    port_byte_out(VGA_ATTRIBUTE_INDEX, i);
    port_byte_out(VGA_ATTRIBUTE_INDEX, i);
  }
  WRITE_ATTRIBUTE(0x10, 0x41); // mode control
  WRITE_ATTRIBUTE(0x11, 0x00); // overscan
  WRITE_ATTRIBUTE(0x12, 0x0F); // color plane enable
  WRITE_ATTRIBUTE(0x13, 0x00); // horizontal panning
  WRITE_ATTRIBUTE(0x14, 0x00); // color select

  _vga_enable_display();

  sti();
}

void init_vga13h() {
  vga13_bb = (uint8_t *)kmalloc(VGA_MODE13_SIZE);
  assert(vga13_bb);
}

void vga13_put_pixel(int x, int y, uint8_t color) {
  if (x < 0 || x >= VGA_MODE13_WIDTH || y < 0 || y >= VGA_MODE13_HEIGHT)
    return;
  vga13_bb[y * VGA_MODE13_WIDTH + x] = color;
}

uint8_t vga13_get_pixel(int x, int y) {
  if (x < 0 || x >= VGA_MODE13_WIDTH || y < 0 || y >= VGA_MODE13_HEIGHT)
    return 0;
  return vga13_bb[y * VGA_MODE13_WIDTH + x];
}

void vga13_clear(uint8_t color) { memset(vga13_bb, color, VGA_MODE13_SIZE); }

void vga13_swap_buffers() {
  memcpy((void *)0xA0000, vga13_bb, VGA_MODE13_SIZE);
}

void vga13_draw_rect(int x, int y, int w, int h, uint8_t color) {
  for (int i = 0; i < w; i++) {
    vga13_put_pixel(x + i, y, color);
    vga13_put_pixel(x + i, y + h - 1, color);
  }
  for (int j = 0; j < h; j++) {
    vga13_put_pixel(x, y + j, color);
    vga13_put_pixel(x + w - 1, y + j, color);
  }
}

void vga13_fill_rect(int x, int y, int w, int h, uint8_t color) {
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      vga13_put_pixel(x + i, y + j, color);
    }
  }
}

void vga13_draw_line(int x0, int y0, int x1, int y1, uint8_t color) {
  int dx = x1 - x0;
  int dy = y1 - y0;
  int sx = dx > 0 ? 1 : -1;
  int sy = dy > 0 ? 1 : -1;
  dx = dx < 0 ? -dx : dx;
  dy = dy < 0 ? -dy : dy;

  int err = (dx > dy ? dx : -dy) / 2;

  while (1) {
    vga13_put_pixel(x0, y0, color);
    if (x0 == x1 && y0 == y1)
      break;
    int e2 = err;
    if (e2 > -dx) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dy) {
      err += dx;
      y0 += sy;
    }
  }
}

// set a palette entry (mode 13h has 256 palette entries)
void vga13_set_palette(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
  port_byte_out(0x3C8, index);
  port_byte_out(0x3C9, r >> 2); // VGA uses 6-bit color
  port_byte_out(0x3C9, g >> 2);
  port_byte_out(0x3C9, b >> 2);
}

void vga13_init_rgb_palette() {
  int i = 0;
  for (int r = 0; r < 6; r++) {
    for (int g = 0; g < 8; g++) {
      for (int b = 0; b < 5; b++) {
        if (i < 256) {
          vga13_set_palette(i++, r * 51, g * 36, b * 63);
        }
      }
    }
  }
}

void vga13_draw_char(int x, int y, char c, font13_t *font) {
  if (c < 0 || !font || !font->bdf)
    return;

  glyph_t *glyph = &font->bdf->glyphs[(uint8_t)c];
  int bytes_per_row = (glyph->width + 7) / 8;

  int base_y =
      y + (font->bdf->glyphs['A'].height - glyph->height) * font->scale_y;

  for (int row = 0; row < glyph->height; row++) {
    for (int col = 0; col < glyph->width; col++) {
      int byte_idx = col / 8;
      int bit_pos = 7 - (col % 8);

      if (glyph->bitmap[row * bytes_per_row + byte_idx] & (1 << bit_pos)) {
        for (int sy = 0; sy < font->scale_y; sy++) {
          for (int sx = 0; sx < font->scale_x; sx++) {
            vga13_put_pixel(x + col * font->scale_x + sx,
                            base_y + row * font->scale_y + sy, font->color);
          }
        }
      }
    }
  }
}

void vga13_draw_string(int x, int y, const char *str, font13_t *font) {
  int ox = 0;

  while (*str) {
    if (*str == '\n') {
      y += font->bdf->glyphs['A'].height * font->scale_y;
      ox = 0;
    } else if (*str == ' ') {
      ox += font->bdf->glyphs['A'].width * font->scale_x;
    } else {
      vga13_draw_char(x + ox, y, *str, font);
      ox += (font->bdf->glyphs[(uint8_t)*str].width + 1) * font->scale_x;
    }
    str++;
  }
}

void vga13_draw_cursor(int x, int y, int clicked) {
  uint8_t color = clicked ? 4 : 15; // red when clicking, white otherwise

  // arrow shape
  for (int i = 0; i < 8; i++) {
    vga13_put_pixel(x, y + i, color);     // vertical line
    vga13_put_pixel(x + i, y + i, color); // diagonal
  }
  for (int i = 0; i < 4; i++) {
    vga13_put_pixel(x + 1 + i, y + i, color); // fill
    vga13_put_pixel(x + 2 + i, y + i + 1, color);
  }

  // outline in black for visibility
  vga13_put_pixel(x + 1, y, 0);
  vga13_put_pixel(x, y + 8, 0);
}

/*******************************
 * VESA
 */

static uint32_t *vesa_fb_addr = NULL;
static uint32_t vesa_fb_width = 0;
static uint32_t vesa_fb_height = 0;
static uint32_t vesa_fb_pitch = 0;
static uint8_t vesa_fb_bpp = 0;
static int vesa_initialized = 0;

int vesa_init(multiboot_info_t *mbi) {
  if (!mbi) {
    return -1;
  }

  if (!(mbi->flags & MULTIBOOT_FLAG_FB)) {
    return -1; // no framebuffer available
  }

  vesa_fb_addr = (uint32_t *)(uint32_t)mbi->framebuffer_addr;
  vesa_fb_width = mbi->framebuffer_width;
  vesa_fb_height = mbi->framebuffer_height;
  vesa_fb_pitch = mbi->framebuffer_pitch;
  vesa_fb_bpp = mbi->framebuffer_bpp;
  vesa_initialized = 1;

  return 0;
}

int vesa_is_available(void) { return vesa_initialized; }

uint32_t vesa_get_width(void) { return vesa_fb_width; }

uint32_t vesa_get_height(void) { return vesa_fb_height; }

uint8_t vesa_get_bpp(void) { return vesa_fb_bpp; }

void vesa_put_pixel(int x, int y, uint32_t color) {
  if (!vesa_fb_addr)
    return;
  if (x < 0 || x >= (int)vesa_fb_width || y < 0 || y >= (int)vesa_fb_height)
    return;

  if (vesa_fb_bpp == 32) {
    vesa_fb_addr[y * (vesa_fb_pitch / 4) + x] = color;
  } else if (vesa_fb_bpp == 24) {
    uint8_t *pixel = (uint8_t *)vesa_fb_addr + y * vesa_fb_pitch + x * 3;
    pixel[0] = color & 0xFF;
    pixel[1] = (color >> 8) & 0xFF;
    pixel[2] = (color >> 16) & 0xFF;
  } else if (vesa_fb_bpp == 16) {
    // RGB565
    uint16_t *pixel =
        (uint16_t *)((uint8_t *)vesa_fb_addr + y * vesa_fb_pitch + x * 2);
    uint16_t r = ((color >> 16) & 0xFF) >> 3;
    uint16_t g = ((color >> 8) & 0xFF) >> 2;
    uint16_t b = (color & 0xFF) >> 3;
    *pixel = (r << 11) | (g << 5) | b;
  }
}

uint32_t vesa_get_pixel(int x, int y) {
  if (!vesa_fb_addr)
    return 0;
  if (x < 0 || x >= (int)vesa_fb_width || y < 0 || y >= (int)vesa_fb_height)
    return 0;

  if (vesa_fb_bpp == 32) {
    return vesa_fb_addr[y * (vesa_fb_pitch / 4) + x];
  } else if (vesa_fb_bpp == 24) {
    uint8_t *pixel = (uint8_t *)vesa_fb_addr + y * vesa_fb_pitch + x * 3;
    return pixel[0] | (pixel[1] << 8) | (pixel[2] << 16);
  }
  return 0;
}

void vesa_clear(uint32_t color) {
  if (!vesa_fb_addr)
    return;

  // fast path for 32bpp
  if (vesa_fb_bpp == 32) {
    uint32_t *row = vesa_fb_addr;
    for (uint32_t y = 0; y < vesa_fb_height; y++) {
      for (uint32_t x = 0; x < vesa_fb_width; x++) {
        row[x] = color;
      }
      row = (uint32_t *)((uint8_t *)row + vesa_fb_pitch);
    }
  } else {
    for (uint32_t y = 0; y < vesa_fb_height; y++) {
      for (uint32_t x = 0; x < vesa_fb_width; x++) {
        vesa_put_pixel(x, y, color);
      }
    }
  }
}

void vesa_fill_rect(int x, int y, int w, int h, uint32_t color) {
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      vesa_put_pixel(x + i, y + j, color);
    }
  }
}

void vesa_draw_rect(int x, int y, int w, int h, uint32_t color) {
  for (int i = 0; i < w; i++) {
    vesa_put_pixel(x + i, y, color);
    vesa_put_pixel(x + i, y + h - 1, color);
  }
  for (int j = 0; j < h; j++) {
    vesa_put_pixel(x, y + j, color);
    vesa_put_pixel(x + w - 1, y + j, color);
  }
}

void vesa_draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
  int dx = x1 - x0;
  int dy = y1 - y0;
  int sx = dx > 0 ? 1 : -1;
  int sy = dy > 0 ? 1 : -1;
  dx = dx < 0 ? -dx : dx;
  dy = dy < 0 ? -dy : dy;

  int err = (dx > dy ? dx : -dy) / 2;

  while (1) {
    vesa_put_pixel(x0, y0, color);
    if (x0 == x1 && y0 == y1)
      break;
    int e2 = err;
    if (e2 > -dx) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dy) {
      err += dx;
      y0 += sy;
    }
  }
}