#pragma once

#include "cpu/isr.h"
#include "libc/types.h"

typedef enum {
  TASK_INACTIVE,
  TASK_WAITING_FOR_START,
  TASK_WAITING,
  TASK_DEAD,
  TASK_RUNNING
} task_status_t;

typedef void (*task_function_t)(void);
typedef struct {
  char name[16];
  task_status_t status;
  task_function_t task; // i dont actually use this for anything
  uint32_t ebp, esp;
  registers_t ctx;
  uint32_t stack_size;
} task_t;

#define MAX_TASK 4
#define STACK_CANARY 0xDEADBEEF

void schedule(registers_t *regs);

void init_tasking();
void create_task(int task_id, char *name, task_function_t f, void *stack,
                 uint32_t stack_size);