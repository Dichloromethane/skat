#include "skat/package.h"

void
free_payload_event(payload_event *p) {
  if (p->ev.type == EVENT_MESSAGE)
	free(p->ev.message);
}

void
free_payload_action(payload_action *p) {}
