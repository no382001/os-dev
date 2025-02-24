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
