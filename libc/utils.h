#pragma once
#include "bits.h"

void sleep(uint32_t milliseconds);
#define assert(c)                                                              \
  do {                                                                         \
    if (c) {                                                                   \
    } else {                                                                   \
      serial_debug("[ASSERT] failed on " #c "!")                               \
    }                                                                          \
  } while (0);
