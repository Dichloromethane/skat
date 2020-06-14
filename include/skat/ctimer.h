#pragma once

#define _GNU_SOURCE

#include <semaphore.h>

typedef struct {
  pthread_t tid;
  timer_t timer_id;
  void (*timerf)(void *);
  void *arg;
  sem_t activations;
  int nsecs;
  int close;
} ctimer;

void ctimer_create(ctimer *, void *, void (*timerf)(void *), int);

void ctimer_run(ctimer *);
void ctimer_stop(ctimer *);
