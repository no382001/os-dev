#pragma once
#include "bits.h"

#define STACK_SIZE 8192
#define STACK_BOTTOM (0x90000 - STACK_SIZE)

uint32_t get_stack_usage();
void print_stack_info();