#include "cpu/idt.h"
#include "cpu/isr.h"
#include "cpu/timer.h"
#include "drivers/screen.h"
#include "drivers/serial.h"
#include "utils.h"
#include "drivers/keyboard.h"

void main(void) {
  kernel_clear_screen();

  serial_init();
  isr_install();

  asm volatile("sti");
  //init_timer(50);
  init_keyboard();

  kernel_puts("hello kernel!\n");
  serial_debug("hello serial!\n");
}
