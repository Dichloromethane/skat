#pragma once

#include "skat/skat.h"
#include "skat/connection.h"
#include "skat/player.h"
#include <pthread.h>

typedef struct server {
  pthread_mutex_t lock;
  connection_s2c conns[4];
  player ps[4];
  skat_state skat_state;
  int ncons;
  pthread_t conn_listener;
  int port;
} server;

int server_has_player_id(server *, player_id *);
connection_s2c *server_get_free_connection(server *);
connection_s2c *server_get_connection_by_pid(server *, player_id);
player *server_get_player_by_pid(server *, player_id);
void server_add_player(server *, player *);
void server_notify_join(server *, player *);
void server_resync_player(server *, player *, skat_client_state *);

void server_acquire_state_lock(server *);
void server_release_state_lock(server *);

void server_disconnect_connection(server *, connection_s2c *);

void server_tick(server *s);

void server_send_event(server *, event *, player *);
void server_distribute_event(server *, event *, void (*)(event *, player *));

void server_init(server *, int);
_Noreturn void server_run(server *);
