#pragma once

#include "libc/types.h"

#define KEY_BACKSPACE 0x0E
#define KEY_ENTER 0x1C
#define KEY_LSHIFT 0x2A
#define KEY_RSHIFT 0x36
#define KEY_LSHIFT_REL 0xAA
#define KEY_RSHIFT_REL 0xB6
#define KEY_LCTRL 0x1D
#define KEY_LCTRL_REL 0x9D
#define KEY_LALT 0x38
#define KEY_LALT_REL 0xB8

typedef void (*key_handler_t)(uint8_t scancode, char ascii, int is_pressed);

typedef struct {
  char key_buffer[256];
  uint8_t buffer_pos;

  uint8_t shift_pressed;
  uint8_t ctrl_pressed;
  uint8_t alt_pressed;

  key_handler_t current_handler;
  key_handler_t default_handler;
} keyboard_ctx_t;

void init_keyboard();
keyboard_ctx_t *get_kb_ctx();
key_handler_t register_key_handler(key_handler_t new_handler);