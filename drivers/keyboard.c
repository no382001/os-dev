#include "keyboard.h"
#include "cpu/isr.h"
#include "libc/string.h"
#include "low_level.h"
#include "screen.h"

static char key_buffer[256];

#define BACKSPACE 0x0E
#define ENTER 0x1C
#define LSHIFT 0x2A
#define RSHIFT 0x36
#define LSHIFT_RELEASE 0xAA
#define RSHIFT_RELEASE 0xB6

static int shift_pressed = 0;

#define SC_MAX 57
const char sc_ascii[] = {
    '?', '?', '1', '2', '3', '4', '5', '6', '7', '8', '9',  '0', '-', '=',  '?',
    '?', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',  '[', ']', '?',  '?',
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', '?', '\\', 'z',
    'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', '?', '?',  '?', ' '};

const char sc_ascii_shifted[] = {
    '?', '?', '!', '@', '#', '$', '%', '^', '&', '*', '(',  ')', '_', '+', '?',
    '?', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',  '{', '}', '?', '?',
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', '?', '|', 'Z',
    'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', '?', '?',  '?', ' '};

void user_input(char *input);

static void keyboard_callback(registers_t *regs) {
  (void)regs;
  uint8_t scancode = port_byte_in(0x60);

  if (scancode == LSHIFT || scancode == RSHIFT) {
    shift_pressed = 1;
    return;
  } else if (scancode == LSHIFT_RELEASE || scancode == RSHIFT_RELEASE) {
    shift_pressed = 0;
    return;
  }

  if (scancode > SC_MAX)
    return;

  if (scancode == BACKSPACE) {
    backspace(key_buffer);
    kernel_put_backspace();
  } else if (scancode == ENTER) {
    kernel_puts("\n");
    user_input(key_buffer);
    key_buffer[0] = '\0';
  } else {
    char letter = shift_pressed ? sc_ascii_shifted[(int)scancode]
                                : sc_ascii[(int)scancode];
    char str[2] = {letter, '\0'};
    append(key_buffer, letter);
    kernel_puts(str);
  }
}

void init_keyboard() { register_interrupt_handler(IRQ1, keyboard_callback); }