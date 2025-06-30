#pragma once
#include "types.h"

typedef struct {
  unsigned long ebx, esi, edi, ebp;
  unsigned long esp;
  unsigned long eip;
} jmp_buf[1];

int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val);