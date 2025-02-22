#pragma once

/*******************************
 * text mode
 */

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

int is_mode_03h();

void kernel_clear_screen();
void kernel_putc(char c);
void kernel_puts(char *message);
void kernel_put_backspace();
void kernel_print_set_attribute(char a);

void kernel_printf(const char *fmt, ...);

#define kernel_aprintf(attr, ...)                                              \
  do {                                                                         \
    kernel_print_set_attribute(attr);                                          \
    kernel_printf(__VA_ARGS__);                                                \
    kernel_print_set_attribute(WHITE_ON_BLACK);                                \
  } while (0);

/*******************************
 * VGA mode 13
 */

#define VGA_MEMORY ((uint8_t *)0xA0000)
#define VGA_WIDTH 640
#define VGA_HEIGHT 480
#define VGA_BUFFER_SIZE ((VGA_WIDTH / 8) * VGA_HEIGHT)

#define VGA_MISC_WRITE 0x3C2
#define VGA_SEQUENCER_INDEX 0x3C4
#define VGA_SEQUENCER_DATA 0x3C5
#define VGA_GRAPHICS_INDEX 0x3CE
#define VGA_GRAPHICS_DATA 0x3CF
#define VGA_CRTC_INDEX 0x3D4
#define VGA_CRTC_DATA 0x3D5
#define VGA_ATTRIBUTE_INDEX 0x3C0
#define VGA_ATTRIBUTE_DATA 0x3C0
#define VGA_INPUT_STATUS 0x3DA

#define WRITE_MISC(val) port_byte_out(VGA_MISC_WRITE, val)
#define WRITE_SEQUENCER(index, val)                                            \
  {                                                                            \
    port_byte_out(VGA_SEQUENCER_INDEX, index);                                 \
    port_byte_out(VGA_SEQUENCER_DATA, val);                                    \
  }
#define WRITE_GRAPHICS(index, val)                                             \
  {                                                                            \
    port_byte_out(VGA_GRAPHICS_INDEX, index);                                  \
    port_byte_out(VGA_GRAPHICS_DATA, val);                                     \
  }
#define WRITE_CRTC(index, val)                                                 \
  {                                                                            \
    port_byte_out(VGA_CRTC_INDEX, index);                                      \
    port_byte_out(VGA_CRTC_DATA, val);                                         \
  }
#define WRITE_ATTRIBUTE(index, val)                                            \
  {                                                                            \
    port_byte_in(VGA_INPUT_STATUS);                                            \
    port_byte_out(VGA_ATTRIBUTE_INDEX, index);                                 \
    port_byte_out(VGA_ATTRIBUTE_DATA, val);                                    \
  }

void set_vga_mode12();
void vga_swap_buffers();

#define VGA_BLACK 0x00
#define VGA_BLUE 0x01
#define VGA_GREEN 0x02
#define VGA_CYAN 0x03
#define VGA_RED 0x04
#define VGA_MAGENTA 0x05
#define VGA_BROWN 0x06
#define VGA_LIGHT_GRAY 0x07
#define VGA_DARK_GRAY 0x08
#define VGA_LIGHT_BLUE 0x09
#define VGA_LIGHT_GREEN 0x0A
#define VGA_LIGHT_CYAN 0x0B
#define VGA_LIGHT_RED 0x0C
#define VGA_LIGHT_MAGENTA 0x0D
#define VGA_YELLOW 0x0E
#define VGA_WHITE 0x0F

void vga_clear_screen(unsigned char color);
void vga_put_pixel(int x, int y, unsigned char color);
void vga_draw_filled_rect(int x, int y, int width, int height, uint8_t color);
void vga_draw_rect(int x, int y, int width, int height, uint8_t color);
void vga_clear_rect(int x, int y, int width, int height);

void draw_bdf_char(int x, int y, char c, uint8_t color);
void draw_bdf_string(int x, int y, const char *str, uint8_t color);