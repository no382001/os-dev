#pragma once
#include "bits.h"

void sleep(uint32_t milliseconds);
#define ASSERT(c)                                                              \
  do {                                                                         \
    if (c) {                                                                   \
    } else {                                                                   \
      serial_debug("assert failed on " #c "!")                                 \
    }                                                                          \
  } while (0);
