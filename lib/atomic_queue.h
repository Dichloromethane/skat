
#pragma once

#include "action.h"
#include "event.h"
#include <pthread.h>

typedef struct aeque_node {
  union {
	action a;
	event e;
  };
  struct aeque_node *next;
} aeque_node;

typedef enum {
  AEQUE_INVALID = 0,
  AEQUE_ACTION,
  AEQUE_EVENT
} aeque_type;

typedef struct {
  pthread_mutex_t lock;
  aeque_node *head;
  aeque_node *tail;
  aeque_type type;
} action_event_queue;

typedef action_event_queue action_queue;
typedef action_event_queue event_queue;

void
init_action_queue(action_event_queue *);
void
init_event_queue(action_event_queue *);
void
enqueue_action(action_event_queue *, action *);
void
enqueue_event(action_event_queue *, event *);
int
dequeue_action(action_event_queue *, action *);
int
dequeue_event(action_event_queue *, event *);
void
clear_action_queue(action_queue *);
void
clear_event_queue(event_queue *);
