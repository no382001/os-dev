#pragma once

#include "cpu/isr.h"
#include "libc/types.h"

typedef enum {
  TASK_INACTIVE,
  TASK_WAITING_FOR_START,
  TASK_WAITING,
  TASK_RUNNING
} task_status_t;

typedef void (*task_function_t)(void);
typedef struct {
  task_status_t status;
  task_function_t task;
  uint32_t ebp, esp;
  registers_t ctx;
} task_t;

#define MAX_TASK 4

void schedule(registers_t *regs);

void init_tasking();
void create_task(int task_id, task_function_t f, void *stack);