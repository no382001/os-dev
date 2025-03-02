#pragma once

#include <stdatomic.h>

typedef struct {
  atomic_int count;
  atomic_flag lock;
} semaphore_t;

void semaphore_init(semaphore_t *sem, int initial_count);
void semaphore_wait(semaphore_t *sem);
void semaphore_signal(semaphore_t *sem);
void acquire(atomic_flag *lock);
void release(atomic_flag *lock);