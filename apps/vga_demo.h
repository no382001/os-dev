#pragma once
#include "bits.h"
#include "libc/bdf.h"

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

void vga13h_plasma_demo();
void vga13h_gradient_demo();