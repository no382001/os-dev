#include "bits.h"

/*
  loads a bdf font from fat16 fs on fs node
*/
int load_bdf(fat_bpb_t *bpb, fs_node_t *file, bdf_font_t *bdf) {
  assert(bpb);
  assert(file);
  assert(bdf);

  uint8_t *buffer = (uint8_t *)kmalloc(file->size);
  if (!buffer)
    return -1;

  fs_read_file(bpb, file, buffer);

  char *line = (char *)buffer;
  char *next_line;
  int current_char = -1;
  int bitmap_line = 0;

  while (line && *line) {
    next_line = strchr(line, '\n');
    if (next_line) {
      *next_line = '\0';
      next_line++;
    }

    if (!strncmp(line, "STARTCHAR", 9)) {
      current_char = -1;
      bitmap_line = 0;
    } else if (!strncmp(line, "ENCODING", 8)) {
      sscanf(line, "ENCODING %d", &current_char);
      if (current_char < 0 || current_char >= MAX_GLYPHS) {
        current_char = -1;
      }
    } else if (!strncmp(line, "BBX", 3) && current_char >= 0) {
      int width, height, xoff, yoff;
      sscanf(line, "BBX %d %d %d %d", &width, &height, &xoff, &yoff);
      bdf->glyphs[current_char].width = width;
      bdf->glyphs[current_char].height = height;
    } else if (!strncmp(line, "BITMAP", 6) && current_char >= 0) {
      bitmap_line = 0;
    } else if (current_char >= 0 && bitmap_line < GLYPH_HEIGHT) {
      unsigned int hex_val;
      if (sscanf(line, "%x", &hex_val) == 1) {
        bdf->glyphs[current_char].bitmap[bitmap_line++] = (uint8_t)hex_val;
        bdf->glyphs[current_char].value = (uint8_t)current_char;
      }
    } else if (!strncmp(line, "ENDCHAR", 7)) {
      current_char = -1;
    }

    line = next_line;
  }

  kfree(buffer);
  return 0;
}