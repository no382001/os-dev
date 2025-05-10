#include "timer.h"
#include "cpu/task.h"
#include "drivers/low_level.h"
#include "drivers/screen.h"
#include "drivers/serial.h"
#include "isr.h"

uint32_t tick = 0;

static void timer_callback(registers_t *regs) {
  (void)regs;
  tick++;
  // schedule(regs);
}

void init_timer(uint32_t freq) {
  register_interrupt_handler(IRQ0, timer_callback);

  /* get PIT value: hardware clock at 1193180 Hz */
  uint32_t divisor = 1193180 / freq;
  uint8_t low = (uint8_t)(divisor & 0xFF);
  uint8_t high = (uint8_t)((divisor >> 8) & 0xFF);

  port_byte_out(0x43, 0x36); /* command port */
  port_byte_out(0x40, low);
  port_byte_out(0x40, high);
}

uint32_t get_tick() { return tick; }