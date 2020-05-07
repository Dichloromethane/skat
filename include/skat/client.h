#pragma once

typedef struct {
} client;

void client_init(client *c, int port);
_Noreturn void client_run(client *c);
