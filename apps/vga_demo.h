#pragma once
#include "bits.h"

typedef struct {
  int offset;
  int x;
  int y;
  int w;
  int h;
  enum { UP = 0, DOWN } scroll;
  enum { LEFT = 0, RIGHT } orient;
} gradient_ctx_t;

void vga12h_gradient_demo();