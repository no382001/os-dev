#pragma once

#include "drivers/keyboard.h"
#include "drivers/screen.h"
#include "fs/fat16.h"
#include "libc/bdf.h"

typedef struct {
  bdf_font_t bdf;
  font_t font;
  uint32_t cursor_x, cursor_y;
  uint32_t height, width;
} terminal_t;

void vga_demo_terminal();
