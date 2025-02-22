#pragma once
#include "drivers/fat16.h"

#define MAX_GLYPHS 256
#define GLYPH_HEIGHT 16
#define GLYPH_WIDTH 8

typedef struct {
  uint8_t value;
  uint8_t width;
  uint8_t height;
  uint8_t bitmap[GLYPH_HEIGHT];
} glyph_t;

typedef struct {
  glyph_t glyphs[MAX_GLYPHS];
} bdf_font_t;

int load_bdf(fat_bpb_t *bpb, fs_node_t *bdf);
glyph_t *get_char_bdf_glyph(char c);