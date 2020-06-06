#pragma once

#define _GNU_SOURCE

#include "skat/connection.h"
#include <pthread.h>

typedef struct client {
  pthread_mutex_t lock;
  pthread_t conn_thread;
  connection_c2s c2s;
  int port;
  char *host;
  char *name;
} client;

void client_acquire_state_lock(client *c);
void client_release_state_lock(client *c);

void client_disconnect_connection(client *c, connection_c2s *conn);
void client_handle_resync(package *p);

void client_init(client *c, char *host, int port, char *name);
_Noreturn void client_run(client *c, int resume);
