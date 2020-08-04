#pragma once

typedef struct async_callback {
  union {
	void *(*do_stuff)(void *);
  };
  void *data;
} async_callback;


#define TYPE async_callback
#include "atomic_queue_header.def"
#undef TYPE

void exec_async(async_callback_queue *q, async_callback *cb);
