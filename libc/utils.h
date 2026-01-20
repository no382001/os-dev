#pragma once
#include "bits.h"

void sleep(uint32_t milliseconds);
#define assert(c)                                                              \
  do {                                                                         \
    if (!(c)) {                                                                \
      serial_debug("========================================");                \
      serial_debug("[ASSERT FAILED] %s", #c);                                  \
      serial_debug("  at %s:%d", __FILE__, __LINE__);                          \
      serial_debug("========================================\n");              \
      kernel_printf("\n[ASSERT FAILED] %s at %s:%d\n", #c, __FILE__,           \
                    __LINE__);                                                 \
      while (1) {                                                              \
        asm volatile("hlt");                                                   \
      }                                                                        \
    }                                                                          \
  } while (0)

void enter_interrupt_context();
void exit_interrupt_context();
bool in_interrupt_context();
void cli();
void sti();