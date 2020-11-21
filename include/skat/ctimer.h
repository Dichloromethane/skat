#pragma once

#include "skat/util.h"
#include <semaphore.h>

typedef struct {
  pthread_t tid;
  char name[THREAD_NAME_SIZE];
  timer_t timer_id;
  void (*timerf)(void *);
  void *arg;
  sem_t activations;
  long nsecs;
  int close;
} ctimer;

void ctimer_create(ctimer *t, const char *name,void *arg, void (*timerf)(void *), long nsecs);
void ctimer_run(ctimer *t);
void ctimer_stop(ctimer *t);
