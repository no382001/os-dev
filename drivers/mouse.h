#pragma once

#include "libc/types.h"

typedef struct {
  int32_t x;
  int32_t y;
  int32_t dx; // delta since last read
  int32_t dy;
  uint8_t buttons;
  uint8_t buttons_changed; // which buttons changed this frame
} mouse_state_t;

typedef void (*mouse_handler_t)(mouse_state_t *state);

typedef struct {
  mouse_state_t state;
  mouse_handler_t current_handler;
  mouse_handler_t default_handler;
  int32_t min_x, max_x;
  int32_t min_y, max_y;
  uint8_t cycle;
  int8_t bytes[3];
} mouse_ctx_t;

void init_mouse(void);
mouse_ctx_t *get_mouse_ctx(void);
mouse_handler_t register_mouse_handler(mouse_handler_t new_handler);
void mouse_set_bounds(int32_t min_x, int32_t max_x, int32_t min_y,
                      int32_t max_y);

mouse_state_t mouse_get_state(void);
int mouse_left_pressed(void);
int mouse_right_pressed(void);
int mouse_middle_pressed(void);
int mouse_left_clicked(void);
int mouse_right_clicked(void);