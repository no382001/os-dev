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

typedef void (*task_fn_t)(void *);
typedef void (*task_destructor_fn_t)(void *);
typedef struct {
  char name[16];
  task_status_t status;
  task_fn_t f;
  void *data;
  task_destructor_fn_t df;
  void *df_data;
  uint32_t ebp, esp;
  registers_t ctx;
  uint32_t stack_size;
} task_t;

#define MAX_TASK 4
#define CANARY 0xBAADF00D

void schedule(registers_t *regs);

void init_tasking();
void create_task(char *name, task_fn_t f, void *data, task_destructor_fn_t df,
                 void *df_data, void *stack, uint32_t stack_size);