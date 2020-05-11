#include "skat/package_queue.h"
#include <stdlib.h>

void
package_queue_init(package_queue *q) {
  q->head = q->tail = NULL;
}

int
package_queue_empty(package_queue *q) {
  return !q->head;
}

void
package_queue_enq(package_queue *q, package *p) {
  package_node *n = malloc(sizeof(package_node));
  n->p = *p;
  n->next = NULL;
  if (package_queue_empty(q)) {
	q->head = q->tail = n;
  } else {
	q->tail->next = n;
	q->tail = n;
  }
}

int
package_queue_deq(package_queue *q, package *p) {
  if (package_queue_empty(q))
	return 0;

  package_node *n = q->head;
  if (q->head == q->tail)
	q->head = q->tail = NULL;
  else
	q->head = q->head->next;

  if (p) {
	*p = n->p;
  }
  free(n);

  return 1;
}

void
package_queue_clear(package_queue *q) {
  while (package_queue_deq(q, NULL)) {
  }
}
