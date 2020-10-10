#include "skat/exec_async.h"

#define ATOMIC_QUEUE_NO_INCLUDE_HEADER
#define TYPE async_callback
#include "atomic_queue.def"
#undef TYPE
#undef ATOMIC_QUEUE_NO_INCLUDE_HEADER

void
exec_async(async_callback_queue *q, async_callback *cb) {
  enqueue_async_callback(q, cb);
}

/*
// client.h:
typedef struct client {
  //...
  async_callback_queue acq;
  //...
} client;

// client_init:
void
client_init(client *c) {
  init_async_callback_queue(&c->acq);
}

// on async exec thread:
void
run(client *c) {
  async_callback acb;
  for (;; dequeue_async_callback_blocking(&c->acq, &acb))
	acb.do_stuff(acb.data);
}

// on io thread (for graphic based):

static void *async_ready2(void *data) {
  client *c = data;
  if (client_is_in_readyable_state(c)) {
	client_ready(c);
  } else {
	global_has_error = 1;
	global_error_message = "Client not readyable";
  }
}

*/

// on io thread (for console based):
/*

static void *
foo(void *data) {
  client *c = data;
  // ...
  return NULL;
}

static void
test(client *c) {
  async_callback acb;

  acb.do_stuff = foo;
  acb.data = c;

  exec_async(&c->acq, &acb);
}
*/
