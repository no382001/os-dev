#include "keyboard.h"
#include "cpu/isr.h"
#include "libc/string.h"
#include "low_level.h"
#include "screen.h"

#define BACKSPACE 0x0E
#define ENTER 0x1C

static char key_buffer[256];

#define SC_MAX 57
const char *sc_name[] = {
    "ERROR",     "Esc",     "1", "2", "3", "4",      "5",
    "6",         "7",       "8", "9", "0", "-",      "=",
    "Backspace", "Tab",     "Q", "W", "E", "R",      "T",
    "Y",         "U",       "I", "O", "P", "[",      "]",
    "Enter",     "Lctrl",   "A", "S", "D", "F",      "G",
    "H",         "J",       "K", "L", ";", "'",      "`",
    "LShift",    "\\",      "Z", "X", "C", "V",      "B",
    "N",         "M",       ",", ".", "/", "RShift", "Keypad *",
    "LAlt",      "Spacebar"};
const char sc_ascii[] = {
    '?', '?', '1', '2', '3', '4', '5', '6', '7', '8', '9',  '0', '-', '=',  '?',
    '?', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',  '[', ']', '?',  '?',
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`', '?', '\\', 'Z',
    'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', '?', '?',  '?', ' '};

void user_input(char *input) {
  if (strcmp(input, "END") == 0) {
    kernel_puts("stopping the CPU. bye!\n");
    asm volatile("hlt");
  }
  kernel_puts("you said: ");
  kernel_puts(input);
  kernel_puts("\n> ");
}

static void keyboard_callback(registers_t *regs) {
  (void)regs;
  uint8_t scancode = port_byte_in(0x60);

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
    char letter = sc_ascii[(int)scancode];
    char str[2] = {letter, '\0'};
    append(key_buffer, letter);
    kernel_puts(str);
  }
}

void init_keyboard() { register_interrupt_handler(IRQ1, keyboard_callback); }