#include "utils.h"
#include "bits.h"

extern uint32_t tick;
void sleep(uint32_t milliseconds) {
  uint32_t target_ticks =
      tick + (milliseconds * 60) / 1000; // (assuming 60Hz PIT)
  while (tick < target_ticks) {
    asm volatile("hlt");
  }
}

volatile int interrupt_nest_level = 0;

void cli() { asm volatile("cli"); }

void enter_interrupt_context() { interrupt_nest_level++; }

void exit_interrupt_context() { interrupt_nest_level--; }

bool in_interrupt_context() { return interrupt_nest_level > 0; }

void sti() {
  if (!in_interrupt_context()) {
    asm volatile("sti");
  }
}