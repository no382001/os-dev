#include "keyboard.h"
#include "low_level.h"
#include "cpu/isr.h"
#include "screen.h"
#include "kernel/utils.h"

void print_letter(u8 scancode);

static void keyboard_callback(registers_t regs) {
    u8 scancode = port_byte_in(0x60);
    char *sc_ascii = 0;
    int_to_ascii(scancode, sc_ascii);
    kernel_puts("keyboard scancode: ");
    kernel_puts(sc_ascii);
    kernel_puts(", ");
    print_letter(scancode);
    kernel_puts("\n");
}

void init_keyboard() {
   register_interrupt_handler(IRQ1, keyboard_callback); 
}

void print_letter(u8 scancode) {
    static char *keymap[] = {
        "ERROR", "ESC", "1", "2", "3", "4", "5", "6", "7", "8",
        "9", "0", "-", "+", "Backspace", "Tab", "Q", "W", "E", "R",
        "T", "Y", "U", "I", "O", "P", "[", "]", "ENTER", "LCtrl",
        "A", "S", "D", "F", "G", "H", "J", "K", "L", ";",
        "'", "`", "LShift", "\\", "Z", "X", "C", "V", "B", "N",
        "M", ",", ".", "/", "RShift", "Keypad *", "LAlt", "Spc"
    };
    
    if (scancode <= 0x39) {
        kernel_puts(keymap[scancode]);
    } else if (scancode <= 0x39 + 0x80) {
        kernel_puts("key up ");
        print_letter(scancode - 0x80);
    } else {
        kernel_puts("unknown key");
    }
}


