#pragma once

#include "drivers/fat16.h"
#include "drivers/keyboard.h"
#include "drivers/screen.h"
#include "libc/bdf.h"

typedef struct {
  bdf_font_t bdf;
  font_t font;
  uint32_t cursor_x, cursor_y;
  uint32_t height, width;
} terminal_t;

void vga_demo_terminal();
