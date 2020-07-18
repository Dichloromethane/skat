#pragma once

#define _GNU_SOURCE

#include "skat/connection.h"
#include "skat/player.h"
#include "skat/skat.h"
#include <pthread.h>

typedef struct server {
  pthread_mutex_t lock;
  skat_state skat_state;
  pthread_t conn_listener;
  int port;
  int ncons;
  connection_s2c conns[4];
  player *ps[4];
  int playermask;
} server;

int server_has_player_id(server *, player_name *);
connection_s2c *server_get_free_connection(server *, int *);
connection_s2c *server_get_connection_by_pname(server *s, player_name *pname,
											   int *n);
connection_s2c *server_get_connection_by_gupid(server *s, int gupid);
player *server_get_player_by_pname(server *s, player_name *pname);
player *server_get_player_by_gupid(server *, int);
void server_add_player_for_connection(server *, player *, int);
void server_notify_join(server *, int gupid);
void server_resync_player(server *, player *, skat_client_state *);

void server_acquire_state_lock(server *);
void server_release_state_lock(server *);

void server_disconnect_connection(server *, connection_s2c *);

void server_tick(server *s);

void server_send_event(server *, event *, player *);
void server_distribute_event(server *, event *, void (*)(event *, player *));

void server_init(server *, int);
_Noreturn void server_run(server *);
