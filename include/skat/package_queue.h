#pragma once

#define _GNU_SOURCE

#include "skat/connection.h"

typedef struct package_node {
  package p;
  struct package_node *next;
} package_node;

typedef struct {
  package_node *head;
  package_node *tail;
} package_queue;

void package_queue_init(package_queue *q);
int package_queue_empty(package_queue *q);
void package_queue_enq(package_queue *q, package *p);
int package_queue_deq(package_queue *q, package *p);
void package_queue_clear(package_queue *q);
