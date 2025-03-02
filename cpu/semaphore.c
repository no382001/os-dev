#include "cpu/semaphore.h"

void semaphore_init(semaphore_t *sem, int initial_count) {
  sem->count = initial_count;
  atomic_flag_clear(&sem->lock);
}

void semaphore_wait(semaphore_t *sem) {
  while (1) {
    asm volatile("cli");

    if (!atomic_flag_test_and_set_explicit(&sem->lock, memory_order_acquire)) {
      if (sem->count > 0) {
        sem->count--;
        atomic_flag_clear_explicit(&sem->lock, memory_order_release);
        break;
      }

      atomic_flag_clear_explicit(&sem->lock, memory_order_release);
    }

    asm volatile("sti");
    asm volatile("int $0x20");
  }
}

void semaphore_signal(semaphore_t *sem) {
  asm volatile("cli");

  if (!atomic_flag_test_and_set_explicit(&sem->lock, memory_order_acquire)) {
    asm volatile("sti");
    asm volatile("int $0x20"); // we dont have it, yield
  }

  sem->count++;

  atomic_flag_clear_explicit(&sem->lock, memory_order_release);

  asm volatile("sti");
  asm volatile("int $0x20");
}

void acquire(atomic_flag *lock) {
  while (atomic_flag_test_and_set_explicit(lock, memory_order_acquire)) {
    __builtin_ia32_pause();
  }
}

void release(atomic_flag *lock) {
  atomic_flag_clear_explicit(lock, memory_order_release);
}
