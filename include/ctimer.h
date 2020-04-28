
#pragma once

#include <semaphore.h>

typedef struct {
  void (*timerf)(void *);
  void *arg;
  timer_t timer_id;
  sem_t activations;
  int nsecs;
} ctimer;

void ctimer_create(ctimer *, void *, void (*timerf)(void *), int);

_Noreturn void ctimer_run(ctimer *);
