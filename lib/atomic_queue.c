#include "atomic_queue.h"
#include <stdlib.h>

static void
init_queue(action_event_queue *q) {
  q->head = q->tail = NULL;
  pthread_mutex_init(&q->lock, NULL);
}

void
init_action_queue(action_event_queue *q) {
  q->type = AEQUE_ACTION;
  init_queue(q);
}

void
init_event_queue(action_event_queue *q) {
  q->type = AEQUE_EVENT;
  init_queue(q);
}

static void
enqueue(action_event_queue *q, aeque_node *n) {
  n->next = NULL;
  pthread_mutex_lock(&q->lock);
  if (!q->head)
	q->head = q->tail = n;
  else {
	q->tail->next = n;
	q->tail = n;
  }
  pthread_mutex_unlock(&q->lock);
}

void
enqueue_action(action_event_queue *q, action *a) {
  aeque_node *n = malloc(sizeof(aeque_node));
  n->a = *a;
  enqueue(q, n);
}

void
enqueue_event(action_event_queue *q, event *e) {
  aeque_node *n = malloc(sizeof(aeque_node));
  n->e = *e;
  enqueue(q, n);
}

static aeque_node *
dequeue(action_event_queue *q) {
  aeque_node *ret;
  if (!q->head)
	return NULL;
  ret = q->head;
  if (q->head == q->tail) {
	q->head = q->tail = 0;
	return ret;
  }
  q->head = q->head->next;
  return ret;
}

int
dequeue_action(action_event_queue *q, action *a) {
  aeque_node *res;
  pthread_mutex_lock(&q->lock);
  res = dequeue(q);
  pthread_mutex_unlock(&q->lock);
  if (!res)
	return 0;
  *a = res->a;
  free(res);
  return 1;
}

int
dequeue_event(action_event_queue *q, event *e) {
  aeque_node *res;
  pthread_mutex_lock(&q->lock);
  res = dequeue(q);
  pthread_mutex_unlock(&q->lock);
  if (!res)
	return 0;
  *e = res->e;
  free(res);
  return 1;
}

static inline void
clear_queue(action_event_queue *q) {
  void *p;
  pthread_mutex_lock(&q->lock);
  while ((p = dequeue(q)))
	free(p);
  pthread_mutex_unlock(&q->lock);
}

void
clear_action_queue(action_queue *q) {
  clear_queue(q);
}

void
clear_event_queue(action_queue *q) {
  clear_queue(q);
}
