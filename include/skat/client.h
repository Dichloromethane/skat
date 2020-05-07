#pragma once

#include <pthread.h>

typedef struct client {
  pthread_mutex_t lock;
  pthread_t conn_thread;
  int port;
  char *host;
} client;

void client_acquire_state_lock(client *c);
void client_release_state_lock(client *c);

void client_init(client *c, char *host, int port);
_Noreturn void client_run(client *c);
