
#include "task.h"
#include "libc/mem.h"

volatile int current_task_idx = 0;
static volatile int active_tasks = 0;
static task_t tasks[MAX_TASK] = {0};

void switch_stack_and_jump(uint32_t stack, uint32_t task);
void switch_stack(uint32_t esp, uint32_t ebp);
#include "cpu/semaphore.h"
extern semaphore_t task_semaphore;

void schedule(registers_t *regs) {
  if (active_tasks == 0 || task_semaphore.count == 0) {
    return;
  }

  int old_task_idx = current_task_idx;
  current_task_idx = (current_task_idx + 1) % MAX_TASK;

  task_t *old_task = &tasks[old_task_idx];
  if (old_task->status == TASK_RUNNING) {
    memcpy(&old_task->ctx, regs, sizeof(registers_t));
  }

  task_t *task_to_run = &tasks[current_task_idx];

  if (task_to_run->status == TASK_WAITING_FOR_START) {
    task_to_run->status = TASK_RUNNING;
    switch_stack_and_jump(task_to_run->esp, (uint32_t)task_to_run->task);
    return;
  } else if (task_to_run->status != TASK_RUNNING) {
    return; // Skip this task
  }

  memcpy(regs, &task_to_run->ctx, sizeof(registers_t));

  // no need for switch_stack - the interrupt return will handle it
}

void create_task(int task_id, task_function_t f, void *stack) {
  task_t task;
  task.status = TASK_WAITING_FOR_START;

  task.esp = (uint32_t)stack - sizeof(registers_t);

  registers_t *initial_context = (registers_t *)task.esp;
  memset(initial_context, 0, sizeof(registers_t));

  initial_context->eip = (uint32_t)f;
  initial_context->cs = 0x08;
  initial_context->eflags = 0x202;
  initial_context->ebp = (uint32_t)stack;

  task.task = f;
  tasks[task_id] = task;
  active_tasks++;
}