
#pragma once

#include "skat/client.h"
#include "skat/event.h"

void *handle_console_input(void *v);
void io_handle_event(client *, event *);
