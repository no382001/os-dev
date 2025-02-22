#include "apps/vga_demo.h"
#include "libc/bdf.h"
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
  load_bdf(&bpb, f);

  while (1) {
    for (int i = 0; i < 4; i++) {
      boxes[i].offset++;
    }

    _draw_scrolling_gradient(&background);

    for (int i = 0; i < 4; i++) {
      _draw_scrolling_gradient(&boxes[i]);
    }

    draw_bdf_string(10, 10, "hello world!", WHITE_ON_BLACK);

    vga_swap_buffers();
    sleep(16);
  }
}
