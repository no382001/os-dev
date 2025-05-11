
#include "task.h"
#include "cpu/semaphore.h"
#include "drivers/keyboard.h"
#include "drivers/serial.h"
#include "libc/mem.h"
#include "libc/utils.h"

#undef serial_debug
#define serial_debug(...)

volatile int current_task_idx = 0;
volatile int active_tasks = 0;
static task_t tasks[MAX_TASK] = {0};

void switch_stack_and_jump(uint32_t stack, uint32_t task);
void switch_stack(uint32_t esp, uint32_t ebp);
extern semaphore_t task_semaphore;

void task_wrapper(task_function_t f) {
  (void)f;
  serial_debug("task %d fired!", current_task_idx);
  task_function_t fn = tasks[current_task_idx].task;
  fn();

  asm volatile("cli");

  tasks[current_task_idx].status = TASK_DEAD;
  active_tasks--;
  serial_debug("task %d completed, now %d active tasks", current_task_idx,
               active_tasks - 1);

  asm volatile("sti");
  // asm volatile("int $0x20");

  // should be never reached
  while (1) {
    asm volatile("hlt");
  }
}

void schedule(registers_t *regs) {
  if (active_tasks < 1) { // besides main
    return;
  }

  int old_task_idx = current_task_idx;
  current_task_idx = (current_task_idx + 1) % MAX_TASK;

  task_t *old_task = &tasks[old_task_idx];
  if (old_task->status == TASK_RUNNING) {
    memcpy(&old_task->ctx, regs, sizeof(registers_t));
  }
  if (current_task_idx >= active_tasks) {
    current_task_idx = 0;
  }
  serial_debug("schedule pass from %s to %s", tasks[old_task_idx].name,
               tasks[current_task_idx].name);

  task_t *task_to_run = &tasks[current_task_idx];

  if (task_to_run->status == TASK_WAITING_FOR_START) {
    task_to_run->status = TASK_RUNNING;
    switch_stack_and_jump(task_to_run->esp, (uint32_t)task_wrapper);
    return;
  } else if (task_to_run->status != TASK_RUNNING) {
    return; // skip this task
  }

  memcpy(regs, &task_to_run->ctx, sizeof(registers_t));

  // no need for switch_stack - the interrupt return will handle it
}

bool check_stack_overflow(int task_id) {
  void *stack_bottom = (void *)(tasks[task_id].ebp - tasks[task_id].stack_size);
  return (*(uint32_t *)stack_bottom != STACK_CANARY);
}

void create_task(int task_id, char *name, task_function_t f, void *stack,
                 uint32_t stack_size) {
  asm volatile("cli");

  task_t task = {0};
  assert(strlen(name) <= 16);
  memcpy(task.name, name, strlen(name));
  task.status = TASK_WAITING_FOR_START;

  uint32_t stack_top = (uint32_t)stack - stack_size;
  uint32_t stack_bottom = (uint32_t)stack; // leave space for canary

  serial_debug("stack_top: %x, stack_bottom: %x, size: %d", stack_top,
               stack_bottom, stack_size);

  *(uint32_t *)stack_bottom = STACK_CANARY;

  task.esp = stack_top - sizeof(registers_t);

  registers_t *initial_context = (registers_t *)task.esp;
  memset(initial_context, 0, sizeof(registers_t));

  initial_context->eip = (uint32_t)task_wrapper;
  initial_context->cs = 0x08;
  initial_context->eflags = 0x202;
  initial_context->esp = task.esp;
  initial_context->ss = 0x10;

  // push parameter for task_wrapper
  task.esp -= 4;
  *(uint32_t *)task.esp = (uint32_t)f;
  task.task = f;
  task.ebp = stack_top;
  task.stack_size = stack_size;

  memcpy((void *)&tasks[task_id], (void *)&task, sizeof(task_t));
  active_tasks++;

  asm volatile("sti");
}

void init_tasking() {
  memset(tasks, 0, sizeof(tasks));

  memcpy(tasks[0].name, "main", 5);
  tasks[0].status = TASK_RUNNING;
  tasks[0].task = NULL;

  current_task_idx = 0;
  active_tasks = 1;
}