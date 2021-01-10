#include "skat/package.h"
#include "skat/str_buf.h"

void
free_payload_event(payload_event *p) {}

void
free_payload_action(payload_action *p) {
  if (p->ac.type == ACTION_MESSAGE)
	str_buf_free(&p->ac.message);
}
