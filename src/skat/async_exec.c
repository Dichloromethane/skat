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
