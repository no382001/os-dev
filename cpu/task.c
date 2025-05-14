
#include "task.h"
#include "apps/hexdump.h"
#include "cpu/semaphore.h"
#include "drivers/keyboard.h"
#include "drivers/serial.h"
#include "libc/mem.h"
#include "libc/utils.h"

/** /
#undef serial_debug
#define serial_debug(...)
/**/

volatile int current_task_idx = 0;
volatile int active_tasks = 0;
task_t tasks[MAX_TASK] = {0};

void switch_stack_and_jump(uint32_t stack, uint32_t task);
void switch_stack(uint32_t esp, uint32_t ebp);
extern semaphore_t task_semaphore;

void task_wrapper(task_fn_t f) {
  (void)f;
  serial_debug("task %d fired!", current_task_idx);
  task_fn_t fn = tasks[current_task_idx].f;
  fn(tasks[current_task_idx].data);

  cli();

  task_destructor_fn_t df = tasks[current_task_idx].df;
  if (df) {
    df(tasks[current_task_idx].df_data);
  }

  tasks[current_task_idx].status = TASK_DEAD;
  active_tasks--;
  serial_debug("task %d completed, now %d active tasks", current_task_idx,
               active_tasks - 1);

  sti();
  // asm volatile("int $0x20");

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

  task_t *task_to_run = &tasks[current_task_idx];

  if (task_to_run->status == TASK_WAITING_FOR_START) {
    task_to_run->status = TASK_RUNNING;
    switch_stack_and_jump(task_to_run->esp, (uint32_t)task_wrapper);
    return;
  } else if (task_to_run->status != TASK_RUNNING) {

  } else if (task_to_run->status != TASK_RUNNING) {
    return; // skip this task
  }
  serial_debug("schedule pass from %d:%s to %d:%s", old_task_idx,
               tasks[old_task_idx].name, current_task_idx,
               tasks[current_task_idx].name);

  memcpy(regs, &task_to_run->ctx, sizeof(registers_t));
}

bool check_stack_overflow(int task_id) {
  void *stack_bottom =
      (void *)(tasks[task_id].ebp - tasks[task_id].stack_size + 4);
  return (*(uint32_t *)stack_bottom != CANARY);
}

int find_next_task_slot() {
  for (int i = 1; i < MAX_TASK; i++) {
    if (tasks[i].status == TASK_INACTIVE || tasks[i].status == TASK_DEAD) {
      return i;
    }
  }
  return -1;
}

void create_task(char *name, task_fn_t f, void *data, task_destructor_fn_t df,
                 void *df_data, void *stack, uint32_t stack_size) {
  assert(f && stack);
  cli();

  int task_id = find_next_task_slot();
  if (task_id == -1) {
    assert(0 && "no more slots for tasks");
    sti();
    return;
  }
  task_t task = {0};
  assert(strlen(name) <= 16);
  memcpy(task.name, name, strlen(name));
  task.status = TASK_WAITING_FOR_START;

  uint32_t stack_bottom = (uint32_t)stack;
  uint32_t stack_top = (uint32_t)stack + stack_size - 4; // canary

  *(uint32_t *)stack_bottom = CANARY;

  task.esp = stack_top - sizeof(registers_t);

  registers_t *initial_context = (registers_t *)task.esp;
  memset(initial_context, 0, sizeof(registers_t));

  initial_context->eip = (uint32_t)task_wrapper;
  initial_context->cs = 0x08;
  initial_context->eflags = 0x202;
  initial_context->esp = task.esp;
  initial_context->ss = 0x10;

  task.f = f;
  task.data = data;
  task.df = df;
  task.df_data = df_data;
  task.ebp = stack_top;
  task.stack_size = stack_size;

  memcpy((void *)&tasks[task_id], (void *)&task, sizeof(task_t));
  active_tasks++;

  serial_debug("task %d created: name='%s', status=%d, function=%x, "
               "stack=%x-%x, esp=%x, ebp=%x, stack_size=%d, canary=%x",
               task_id, task.name, task.status, (uint32_t)task.f, stack_bottom,
               stack_top, task.esp, task.ebp, task.stack_size, CANARY);
  // hexdump(stack, 16, 8);
  sti();
}

void task_stack_check() {
  while (1) {
    // dont do anything with main
    for (int i = 2; i < active_tasks; i++) {
      if (check_stack_overflow(i)) {
        serial_printff("stack overflow in task %d:%s ebp was at %x", i,
                       tasks[i].name, tasks[i].ebp);
        cli();
        void *stack_bottom = (void *)(tasks[i].ebp - tasks[i].stack_size);
        hexdump(stack_bottom, 64, 8);
        while (1) {
        }
      }
    }
  }
}

void kheap_watchdog();
void init_tasking() {
  memset(tasks, 0, sizeof(tasks));

  memcpy(tasks[0].name, "main", 5);
  tasks[0].status = TASK_RUNNING;

  current_task_idx = 0;
  active_tasks = 1;

  // the PF is here
  /*
  void *ss = kmalloc(1024);
  create_task("task_stack_check", task_stack_check, ss, 0, 0, 0, 1024);

  void *s2 = kmalloc(5000);
  create_task("kheap_watchdog", kheap_watchdog, s2, 0, 0, 0, 5000);
  */
}