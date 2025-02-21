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