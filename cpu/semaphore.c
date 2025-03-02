#include "cpu/semaphore.h"

void semaphore_init(semaphore_t *sem, int initial_count) {
  sem->count = initial_count;
  atomic_flag_clear(&sem->lock);
}

void semaphore_wait(semaphore_t *sem) { // this in itself does not guarantee
                                        // that we dont get interrupted
  while (1) {
    while (
        atomic_flag_test_and_set_explicit(&sem->lock, memory_order_acquire)) {
      __builtin_ia32_pause();
    }

    if (sem->count > 0) {
      sem->count--;
      atomic_flag_clear_explicit(&sem->lock, memory_order_release);
      break;
    }

    atomic_flag_clear_explicit(&sem->lock, memory_order_release);

    asm volatile("int $0x20");
  }
}

void semaphore_signal(semaphore_t *sem) {
  while (atomic_flag_test_and_set_explicit(&sem->lock, memory_order_acquire)) {
    __builtin_ia32_pause();
  }

  sem->count++;

  atomic_flag_clear_explicit(&sem->lock, memory_order_release);

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
