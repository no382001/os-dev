#include "apps/vga_demo_terminal.h"
#include "libc/heap.h"
#include "libc/utils.h"

static void terminal_key_handler(uint8_t scancode, char ascii, int is_pressed);

static terminal_t g_t = {0};

static uint32_t g_font_width, g_font_height;

void init_terminal(terminal_t *t) {

  fat_bpb_t bpb = {0};
  fat16_read_bpb(&bpb);
  fs_node_t *root = fs_build_tree(&bpb, 0, 0, 1);

  fs_node_t *f = fs_find_file(root, "VIII.BDF");
  assert(f);
  load_bdf(&bpb, f, &t->bdf);

  fs_cleanup(root);

  init_font(&t->font, &t->bdf);
  t->font.scale_x = 2;
  t->font.scale_y = 2;

  t->cursor_x = 1;
  t->cursor_y = 1;

  register_key_handler(terminal_key_handler);

#define padding_x 3
#define padding_y 3

  g_font_height = padding_y + t->bdf.glyphs['A'].height * t->font.scale_x;
  g_font_width = padding_x + t->bdf.glyphs['A'].width * t->font.scale_y;
}

extern uint8_t *vga_bb;
void vga_scroll_up(int pixels, uint8_t fill_color) {
  int bytes_per_row = VGA_WIDTH / 8;
  int rows_to_scroll = pixels;
  int bytes_to_scroll = rows_to_scroll * bytes_per_row;

  if (bytes_to_scroll >= VGA_BUFFER_SIZE) {
    vga_clear_screen(fill_color);
    return;
  }

  for (int plane = 0; plane < 4; plane++) {
    uint8_t *plane_buffer = &vga_bb[plane * VGA_BUFFER_SIZE];

    memmove(plane_buffer, plane_buffer + bytes_to_scroll,
            VGA_BUFFER_SIZE - bytes_to_scroll);

    uint8_t fill_value = (fill_color & (1 << plane)) ? 0xFF : 0x00;
    memset(plane_buffer + (VGA_BUFFER_SIZE - bytes_to_scroll), fill_value,
           bytes_to_scroll);
  }
}

static char terminal_buffer[256] = {0};
static uint32_t buffer_pos = 0;

static void terminal_backspace(void) {
  g_t.cursor_x -= g_font_width;

  if (g_t.cursor_x < 1) {
    g_t.cursor_x = 1;
  }

  vga_draw_filled_rect(g_t.cursor_x, g_t.cursor_y, g_font_width, g_font_height,
                       VGA_BLACK);
}

static void terminal_newline(void) {
  g_t.cursor_x = 1;
  g_t.cursor_y += g_font_height;

  uint32_t screen_height = VGA_HEIGHT;
  if (g_t.cursor_y + g_font_height > screen_height - 10) {
    vga_scroll_up(g_font_height, 0);
    g_t.cursor_y -= g_font_height;
  }
}

static void terminal_print_char(char c) {
  char str[2] = {c, '\0'};

  draw_bdf_string(g_t.cursor_x, g_t.cursor_y, str, &g_t.font);

  g_t.cursor_x += g_font_width;

  uint32_t screen_width = VGA_HEIGHT;
  if (g_t.cursor_x + g_font_width > screen_width) {
    terminal_newline();
  }
}

static void terminal_process_command(const char *cmd) {
  draw_bdf_string(g_t.cursor_x, g_t.cursor_y, cmd, &g_t.font);
  terminal_newline();
}

static void terminal_redraw_line_from_position(uint32_t start_pos) {
  uint32_t original_x = g_t.cursor_x;
  uint32_t i;

  // clear the rest of the line
  vga_draw_filled_rect(g_t.cursor_x, g_t.cursor_y, VGA_WIDTH - g_t.cursor_x,
                       g_font_height, VGA_BLACK);

  // redraw from the current position to the end
  for (i = start_pos; i < (uint32_t)strlen(terminal_buffer); i++) {
    char str[2] = {terminal_buffer[i], '\0'};
    draw_bdf_string(g_t.cursor_x, g_t.cursor_y, str, &g_t.font);
    g_t.cursor_x += g_font_width;
  }

  g_t.cursor_x = original_x;
}

static void terminal_key_handler(uint8_t scancode, char ascii, int is_pressed) {

  uint32_t prev_pos = g_t.cursor_x;
  if (!is_pressed) {
    return;
  }

  if (scancode == KEY_BACKSPACE) {
    if (buffer_pos > 0) {
      buffer_pos--;
      if (buffer_pos ==
          (uint32_t)strlen(terminal_buffer) - 1) { // its just the end
        terminal_buffer[buffer_pos] = '\0';
        terminal_backspace();
      } else {
        memmove(&terminal_buffer[buffer_pos], &terminal_buffer[buffer_pos + 1],
                strlen(terminal_buffer) - buffer_pos);
        terminal_backspace();
        terminal_redraw_line_from_position(buffer_pos);
      }
    }
  } else if (scancode == KEY_ENTER) {
    terminal_newline();

    terminal_process_command(terminal_buffer);

    buffer_pos = 0;
    terminal_buffer[0] = '\0';

  } else if (scancode == KEY_LEFT) {
    if (buffer_pos > 0) {
      g_t.cursor_x -= g_font_width;
      buffer_pos--;
    }
  } else if (scancode == KEY_RIGHT) {
    if (buffer_pos < (uint32_t)strlen(terminal_buffer)) {
      g_t.cursor_x += g_font_width;
      buffer_pos++;
    }
  } else if (ascii >= ' ' && ascii <= '~') {
    if (buffer_pos < sizeof(terminal_buffer) - 1) {
      if (buffer_pos == (uint32_t)strlen(terminal_buffer)) {
        terminal_buffer[buffer_pos++] = ascii;
        terminal_buffer[buffer_pos] = '\0';
        terminal_print_char(ascii);
      } else {
        // make room for the new character
        memmove(&terminal_buffer[buffer_pos + 1], &terminal_buffer[buffer_pos],
                strlen(terminal_buffer) - buffer_pos + 1);

        terminal_buffer[buffer_pos] = ascii;

        // redraw from the current position
        uint32_t save_pos = g_t.cursor_x;
        terminal_redraw_line_from_position(buffer_pos);
        g_t.cursor_x = save_pos + g_font_width;
        buffer_pos++;
      }
    }
  }
  // serial_debug("%d, %d, %x", buffer_pos, strlen(terminal_buffer), scancode);

  if (g_t.cursor_x != prev_pos) {
    vga_draw_rect(prev_pos, g_t.cursor_y, g_font_width, g_font_height,
                  VGA_BLACK);
    vga_draw_rect(g_t.cursor_x, g_t.cursor_y, g_font_width, g_font_height,
                  WHITE_ON_BLACK);
  }
}

void vga_demo_terminal() {
  init_terminal(&g_t);
  vga_clear_screen(0);
  while (1) {
    vga_swap_buffers();
    sleep(16);
  }
}