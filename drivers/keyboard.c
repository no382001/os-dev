#include "keyboard.h"
#include "apps/hexdump.h"
#include "cpu/isr.h"
#include "libc/string.h"
#include "low_level.h"
#include "screen.h"

keyboard_ctx_t kb_ctx = {0};

#define SC_MAX 57
const char sc_ascii[] = {
    0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9',  '0', '-', '=',  0,
    0,   'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',  '[', ']', 0,    0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,   '\\', 'z',
    'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,   0,    0,   ' '};

const char sc_ascii_shifted[] = {
    0,   0,   '!', '@', '#', '$', '%', '^', '&', '*', '(',  ')', '_', '+', 0,
    0,   'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',  '{', '}', 0,   0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0,   '|', 'Z',
    'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,   0,    0,   ' '};

static char scancode_to_ascii(uint8_t scancode, int shift_pressed) {
  if (scancode > SC_MAX)
    return 0;

  return shift_pressed ? sc_ascii_shifted[scancode] : sc_ascii[scancode];
}

static void default_key_handler(uint8_t scancode, char ascii, int is_pressed);

static void keyboard_callback(registers_t *regs) {
  (void)regs;
  uint8_t scancode = port_byte_in(0x60);
  int is_pressed = !(scancode & 0x80);
  uint8_t normalized_scancode = is_pressed ? scancode : (scancode & 0x7F);

  // serial_debug("%x,%x", is_pressed, normalized_scancode);
  if (normalized_scancode == KEY_LSHIFT || normalized_scancode == KEY_RSHIFT) {
    kb_ctx.shift_pressed = is_pressed;
    return;
  } else if (normalized_scancode == KEY_LCTRL) {
    kb_ctx.ctrl_pressed = is_pressed;
    return;
  } else if (normalized_scancode == KEY_LALT) {
    kb_ctx.alt_pressed = is_pressed;
    return;
  }

  if (!is_pressed) {
    return;
  }

  char ascii = 0;
  if (normalized_scancode <= SC_MAX) {
    ascii = scancode_to_ascii(normalized_scancode, kb_ctx.shift_pressed);
  }

  if (kb_ctx.current_handler) {
    kb_ctx.current_handler(normalized_scancode, ascii, is_pressed);
  }
}

void clear_key_buffer(void) {
  kb_ctx.buffer_pos = 0;
  memset(kb_ctx.key_buffer, 0, sizeof(kb_ctx.key_buffer));
}

static void default_key_handler(uint8_t scancode, char ascii, int is_pressed) {
  if (!is_pressed) {
    return;
  }

  if (scancode == KEY_BACKSPACE) {
    if (kb_ctx.buffer_pos > 0) {
      kb_ctx.buffer_pos--;
      kb_ctx.key_buffer[kb_ctx.buffer_pos] = '\0';
      kernel_put_backspace();
    }
  } else if (scancode == KEY_ENTER) {
    kb_ctx.key_buffer[kb_ctx.buffer_pos] = '\0';

    if (kb_ctx.enter_handler) {
      // serial_debug("buffer contents: '%s' (length: %d)", kb_ctx.key_buffer,
      // kb_ctx.buffer_pos);
      kb_ctx.enter_handler(kb_ctx.key_buffer);
    } else {
      kernel_puts("\n");
    }
    clear_key_buffer();
  } else if (ascii != 0) {
    if (kb_ctx.buffer_pos < sizeof(kb_ctx.key_buffer) - 1) {
      kb_ctx.key_buffer[kb_ctx.buffer_pos] = ascii;
      kb_ctx.buffer_pos++;
      kb_ctx.key_buffer[kb_ctx.buffer_pos] = '\0';

      char str[2] = {ascii, '\0'};
      kernel_puts(str);

      // serial_debug("added '%c' to buffer, pos now %d", ascii,
      // kb_ctx.buffer_pos);
    } else {
      // serial_debug("buffer full! Cannot add '%c'", ascii);
    }
  }
}

key_handler_t register_key_handler(key_handler_t new_handler) {
  key_handler_t old_handler = kb_ctx.current_handler;
  kb_ctx.current_handler = new_handler;
  return old_handler;
}

void init_keyboard(void) {
  kb_ctx.default_handler = default_key_handler;
  kb_ctx.current_handler = default_key_handler;
  kb_ctx.enter_handler = 0;
  kb_ctx.buffer_pos = 0;
  kb_ctx.shift_pressed = 0;
  kb_ctx.ctrl_pressed = 0;
  kb_ctx.alt_pressed = 0;

  register_interrupt_handler(IRQ1, keyboard_callback);
  kernel_printf("- keyboard installed\n");
}

keyboard_ctx_t *get_kb_ctx(void) { return &kb_ctx; }
