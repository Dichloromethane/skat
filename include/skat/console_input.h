
#pragma once

#include "skat/client.h"
#include "skat/event.h"

_Noreturn void *handle_console_input(void *cc);
void io_handle_event(client *, event *);
