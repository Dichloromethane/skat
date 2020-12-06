#pragma once

#include "skat/connection.h"
#include "skat/ctimer.h"
#include "skat/package.h"
#include "skat/player.h"
#include "skat/skat.h"
#include <netinet/in.h>
#include <pthread.h>

typedef struct {
  int socket_fd;
  struct sockaddr_in addr;
} server_listener_args;

typedef struct server {
  int exit;
  pthread_mutex_t lock;
  skat_server_state ss;
  ctimer tick_timer;
  pthread_t signal_listener;
  pthread_t conn_listener;
  server_listener_args listener;
  int port;
  int ncons;
  connection_s2c conns[4];
  player *pls[4];
  int playermask;
} server;

int server_is_player_active(server *s, int8_t gupid);
int server_has_player_name(server *s, char *pname);
connection_s2c *server_get_free_connection(server *s, int8_t *gupid_out);
connection_s2c *server_get_connection_by_pname(server *s, char *pname,
											   int8_t *gupid_out);
connection_s2c *server_get_connection_by_gupid(server *s, int8_t gupid);
void server_add_player_for_connection(server *s, player *p, int8_t gupid);
void server_resume_player_for_connection(server *s, int8_t gupid);
void server_notify_join(server *s, int8_t gupid);
void server_resync_player(server *s, player *p, payload_resync *pl_rs);

void server_acquire_state_lock(server *);
void server_release_state_lock(server *);

void server_disconnect_connection(server *, connection_s2c *);

void server_tick(server *s);

void server_send_event(server *, event *, player *);
void server_distribute_event(server *, event *, void (*)(event *, player *));

void server_init(server *, int);
_Noreturn void server_run(server *);
