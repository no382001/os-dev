#include "apps/vga_demo.h"
extern uint8_t *vga_bb;

void _draw_scrolling_gradient(gradient_ctx_t *ctx) {
  for (int plane = 0; plane < 4; plane++) {
    uint8_t *plane_buffer = (uint8_t *)&vga_bb[plane * VGA_BUFFER_SIZE];

    for (int y = 0; y < ctx->h; y++) {
      int screen_y = ctx->y + y;
      if (screen_y >= VGA_HEIGHT)
        break;

      for (int x = 0; x < (ctx->w / 8); x++) {
        int screen_x = ctx->x + x * 8;
        if (screen_x >= VGA_WIDTH)
          break;

        uint8_t color_byte = 0;

        for (int bit = 0; bit < 8; bit++) {
          int pixel_x = screen_x + bit;
          int adjusted_x = pixel_x;
          int adjusted_y = screen_y;

          if (ctx->scroll == DOWN) {
            // what? why does it only move like this?
            adjusted_y -= ctx->offset * 2;
          } else {
            adjusted_y += ctx->offset;
          }

          if (ctx->orient == RIGHT) {
            adjusted_x += ctx->offset;
          } else {
            adjusted_x -= ctx->offset;
          }

          int pixel_value;
          if (ctx->orient == RIGHT) {
            pixel_value = ((adjusted_x + adjusted_y) % VGA_WIDTH) / 40;
          } else {
            pixel_value = ((adjusted_x - adjusted_y) % VGA_WIDTH) / 40;
          }

          if (pixel_value & (1 << plane)) {
            color_byte |= (1 << bit);
          }
        }

        plane_buffer[(screen_y * (VGA_WIDTH / 8)) + (screen_x / 8)] =
            color_byte;
      }
    }
  }
}

void vga12h_gradient_demo() {
  vga_bb = (uint8_t *)kmalloc(VGA_BUFFER_SIZE * 4);
  vga_clear_screen(0);

  gradient_ctx_t background = {.offset = 0,
                               .x = 0,
                               .y = 0,
                               .w = VGA_WIDTH,
                               .h = VGA_HEIGHT,
                               .scroll = DOWN,
                               .orient = LEFT};

  gradient_ctx_t boxes[4] = {{.offset = 35,
                              .x = 50,
                              .y = 50,
                              .w = VGA_WIDTH / 4,
                              .h = VGA_HEIGHT / 4,
                              .scroll = UP,
                              .orient = LEFT},
                             {.offset = 35,
                              .x = VGA_WIDTH / 2,
                              .y = 50,
                              .w = VGA_WIDTH / 4,
                              .h = VGA_HEIGHT / 4,
                              .scroll = UP,
                              .orient = RIGHT},
                             {.offset = 35,
                              .x = 50,
                              .y = VGA_HEIGHT / 2,
                              .w = VGA_WIDTH / 4,
                              .h = VGA_HEIGHT / 4,
                              .scroll = DOWN,
                              .orient = LEFT},
                             {.offset = 35,
                              .x = VGA_WIDTH / 2,
                              .y = VGA_HEIGHT / 2,
                              .w = VGA_WIDTH / 4,
                              .h = VGA_HEIGHT / 4,
                              .scroll = DOWN,
                              .orient = RIGHT}};

  fat_bpb_t bpb = {0};
  fat16_read_bpb(&bpb);
  fs_node_t *root = fs_build_tree(&bpb, 0, 0, 1);
  // fs_node_t *f = fs_find_file(root, "TOM-TH~1.BDF");
  fs_node_t *f = fs_find_file(root, "VIII.BDF");
  bdf_font_t bdf = {0};
  load_bdf(&bpb, f, &bdf);

  font_t font = {0};
  init_font(&font, &bdf);

  font.scale_x = 5;
  font.scale_y = 25;

  int x = 20, y = 20;
  char string[] = "Hello World!";

  while (1) {
    for (int i = 0; i < 4; i++) {
      boxes[i].offset++;
    }

    _draw_scrolling_gradient(&background);

    for (int i = 0; i < 4; i++) {
      _draw_scrolling_gradient(&boxes[i]);
    }

    draw_bdf_string(x, y, string, &font);
    font.color = BLUE_ON_BLACK;
    draw_bdf_string(x + 20, y + 20, string, &font);
    font.color = WHITE_ON_BLACK;

    vga_swap_buffers();
    sleep(16);
  }
}

void vga12h_terminal_demo() {}

void vga13h_plasma_demo() {
  init_vga13h();
  set_vga_mode13();

  for (int i = 0; i < 64; i++) {
    // black to red
    vga13_set_palette(i, i * 4, 0, 0);
    // red to yellow
    vga13_set_palette(64 + i, 255, i * 4, 0);
    // yellow to green to cyan
    vga13_set_palette(128 + i, 255 - i * 4, 255, i * 4);
    // cyan to black
    vga13_set_palette(192 + i, 0, 255 - i * 4, 255 - i * 4);
  }

  int offset = 0;

  while (1) {
    for (int y = 0; y < VGA_MODE13_HEIGHT; y++) {
      for (int x = 0; x < VGA_MODE13_WIDTH; x++) {

        int v1 = (x + offset) % 64;
        int v2 = (y + offset / 2) % 64;
        int v3 = ((x + y + offset) / 2) % 64;
        int v4 = ((x - y + offset + 256) / 2) % 64;

        uint8_t color = (v1 + v2 + v3 + v4);
        vga13_put_pixel(x, y, color);
      }
    }

    vga13_swap_buffers();
    offset++;
    sleep(16 * 2);
  }
}

void vga13h_gradient_demo() {
  init_vga13h();
  set_vga_mode13();

  for (int i = 0; i < 256; i++) {
    int r = (i * 4) % 256;
    int g = (i * 2) % 256;
    int b = (i * 3) % 256;
    vga13_set_palette(i, r, g, b);
  }

  fat_bpb_t bpb = {0};
  fat16_read_bpb(&bpb);
  fs_node_t *root = fs_build_tree(&bpb, 0, 0, 1);
  fs_node_t *f = fs_find_file(root, "VIII.BDF");
  bdf_font_t bdf = {0};
  load_bdf(&bpb, f, &bdf);

  font13_t font = {.bdf = &bdf, .color = 255, .scale_x = 2, .scale_y = 2};

  int offset = 0;
  char string[] = "Mode 13h!";

  struct {
    int x, y, dx, dy, w, h, color;
  } boxes[4] = {{20, 20, 2, 1, 40, 30, 32},
                {200, 30, -1, 2, 50, 40, 64},
                {100, 100, 1, -1, 35, 35, 128},
                {150, 80, -2, -1, 45, 25, 200}};

  while (1) {
    for (int y = 0; y < VGA_MODE13_HEIGHT; y++) {
      for (int x = 0; x < VGA_MODE13_WIDTH; x++) {
        uint8_t color = ((x + offset) ^ (y + offset / 2)) & 0xFF;
        vga13_put_pixel(x, y, color);
      }
    }

    for (int i = 0; i < 4; i++) {
      boxes[i].x += boxes[i].dx;
      boxes[i].y += boxes[i].dy;

      if (boxes[i].x <= 0 || boxes[i].x + boxes[i].w >= VGA_MODE13_WIDTH)
        boxes[i].dx = -boxes[i].dx;
      if (boxes[i].y <= 0 || boxes[i].y + boxes[i].h >= VGA_MODE13_HEIGHT)
        boxes[i].dy = -boxes[i].dy;

      vga13_fill_rect(boxes[i].x, boxes[i].y, boxes[i].w, boxes[i].h,
                      boxes[i].color);
      vga13_draw_rect(boxes[i].x, boxes[i].y, boxes[i].w, boxes[i].h, 255);
    }

    vga13_draw_string(10, 10, string, &font);
    font.color = 128;
    vga13_draw_string(12, 12, string, &font);
    font.color = 255;

    vga13_swap_buffers();
    offset++;
    sleep(16 * 2);
  }
}