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

void client_prepare_exit(client *c);
void client_handle_resync(client *c, payload_resync *pl);
void client_notify_join(client *, payload_notify_join *);
void client_notify_leave(client *, payload_notify_leave *);

void client_ready(client *c, client_action_callback *);
void client_play_card(client *c, card_id cid, client_action_callback *);
void client_set_gamerules(client *c, game_rules gr, client_action_callback *);

void client_reizen_confirm(client *, client_action_callback *);
void client_reizen_passe(client *, client_action_callback *);
void client_reizen_number(client *c, uint16_t next_reizwert,
						  client_action_callback *cac);

void client_skat_take(client *, client_action_callback *);
void client_skat_leave(client *, client_action_callback *);
void client_skat_press(client *, card_id, card_id, client_action_callback *);

void client_say(client *, str_buf);

void client_init(client *c, char *host, int port, char *name);
void client_run(client *c, int resume);
