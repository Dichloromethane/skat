#pragma once

#include "skat/connection.h"
#include "skat/exec_async.h"
#include "skat/package.h"
#include "skat/skat.h"
#include "skat/util.h"
#include <pthread.h>

struct client;
typedef struct client client;

typedef struct {
  event e;
  client *c;
} client_action_callback_hdr;

typedef struct {
  void (*f)(void *);
  void *args;
} client_action_callback;

#define TYPE   client_action_callback
#define HEADER 1
#define ATOMIC 1
#include "fast_ll.def"
#undef ATOMIC
#undef HEADER
#undef TYPE

struct client {
  pthread_mutex_t lock;
  pthread_t signal_listener;
  pthread_t conn_thread;
  pthread_t exec_async_handler;
  pthread_t io_handler;
  async_callback_queue acq;
  connection_c2s c2s;
  int exit;
  int port;
  char *host;
  char *name;
  skat_client_state cs;
  player *pls[4];
  ll_client_action_callback ll_cac;
};

void client_acquire_state_lock(client *c);
void client_release_state_lock(client *c);

void client_disconnect_connection(client *c, connection_c2s *conn);
void client_handle_resync(client *c, payload_resync *pl);
void client_notify_join(client *, payload_notify_join *);
void client_notify_leave(client *, payload_notify_leave *);

void client_ready(client *c, client_action_callback *);
void client_play_card(client *c, unsigned int card_index,
					  client_action_callback *);

void client_init(client *c, char *host, int port, char *name);
_Noreturn void client_run(client *c, int resume);
