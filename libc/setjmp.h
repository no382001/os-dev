#pragma once
#include "types.h"

typedef struct {
  uint32_t eax, ebx, ecx, edx;
  uint32_t esi, edi, esp, ebp;
  uint32_t eip, eflags;
} jmp_buf_struct;

typedef jmp_buf_struct jmp_buf[1];

int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val);