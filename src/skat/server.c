#include "skat/server.h"
#include "skat/ctimer.h"
#include "skat/util.h"
#include "conf.h"
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

int
server_has_player_id(server *s, player_id *pid) {
  for (int i = 0; i < s->ncons; i++)
	if (player_id_equals(&s->ps[i].id, pid))
	  return 1;
  return 0;
}

void
server_send_event(server *s, event *e, player *pl) {
  connection_s2c *c = server_get_connection_by_pid(s, pl->id);
  if (c) {
	conn_enqueue_event(c, e);
  }
}

void
server_distribute_event(server *s, event *ev,
						void (*mask_event)(event *, player *)) {
  event e;
  DEBUG_PRINTF("Distributing event of type %s", event_name_table[ev->type]);
  ;
  for (int i = 0; i < s->ncons; i++) {
	if (mask_event) {
	  e = *ev;
	  mask_event(&e, &s->ps[i]);
	  server_send_event(s, &e, &s->ps[i]);
	} else
	  server_send_event(s, ev, &s->ps[i]);
  }
}

connection_s2c *
server_get_free_connection(server *s) {
  if (s->ncons > 4)
	return NULL;
  return &s->conns[s->ncons++];
}

void
server_add_player(server *s, player *pl) {
  memcpy(&s->ps[s->ncons - 1], pl, sizeof(player));
}

connection_s2c *
server_get_connection_by_pid(server *s, player_id pid) {
  for (int i = 0; i < s->ncons; i++)
	if (player_id_equals(&s->ps[i].id, &pid))
	  return &s->conns[i];
  return NULL;
}

player *
server_get_player_by_pid(server *s, player_id pid) {
  for (int i = 0; i < s->ncons; i++)
	if (player_id_equals(&s->ps[i].id, &pid))
	  return &s->ps[i];
  return NULL;
}

void
server_disconnect_connection(server *s, connection_s2c *c) {
  player *pl;
  pl = server_get_player_by_pid(s, c->pid);

  skat_state_notify_disconnect(&s->skat_state, pl, s);
  for (int i = 0; i < s->ncons; i++)
	if (!player_equals_by_id(pl, &s->ps[i]))
	  conn_notify_disconnect(&s->conns[i], pl);
  conn_disable_conn(c);
}

void
server_acquire_state_lock(server *s) {
  DEBUG_PRINTF("Acquiring server state lock from thread %d", gettid());
  pthread_mutex_lock(&s->lock);
  DEBUG_PRINTF("Acquired server state lock from thread %d", gettid());
}

void
server_release_state_lock(server *s) {
  DEBUG_PRINTF("Releasing server state lock from thread %d", gettid());
  pthread_mutex_unlock(&s->lock);
  DEBUG_PRINTF("Released server state lock from thread %d", gettid());
}

void
server_resync_player(server *s, player *pl, skat_client_state *cs) {
  DEBUG_PRINTF("Resync requested by %s", pl->id.str);
  skat_resync_player(cs, pl);
}

void
server_tick(server *s) {
  DEBUG_PRINTF("Server tick");

  action a;
  event err_ev;
  server_acquire_state_lock(s);
  for (int i = 0; i < s->ncons; i++) {
	if (!s->conns[i].active)
	  continue;

	while (conn_dequeue_action(&s->conns[i], &a)) {
	  if (!skat_state_apply(&s->skat_state, &a, &s->ps[i], s)) {
		DEBUG_PRINTF("Received illegal action of type %s from player %s with "
					 "id %ld, rejecting",
					 action_name_table[a.type], s->ps[i].id.str, a.id);
		err_ev.type = EVENT_ILLEGAL_ACTION;
		err_ev.answer_to = a.id;
		copy_player_id(&err_ev.player, &s->ps[i].id);
		conn_enqueue_event(&s->conns[i], &err_ev);
	  }
	}
	skat_state_tick(&s->skat_state, s);
  }
  server_release_state_lock(s);
}

void
server_notify_join(server *s, player *pl) {
  DEBUG_PRINTF("Player %s joined", pl->id.str);
  skat_state_notify_join(&s->skat_state, pl, s);
  for (int i = 0; i < s->ncons; i++)
	if (!player_equals_by_id(pl, &s->ps[i]))
	  conn_notify_join(&s->conns[i], pl);
}

typedef struct {
  server *s;
  int conn_fd;
} server_handler_args;

static void *
server_handler(void *args) {
  connection_s2c *conn;
  server_handler_args *hargs = args;
  DEBUG_PRINTF("Handler started, establishing connection with %d",
			   hargs->conn_fd);
  conn = establish_connection_server(hargs->s, hargs->conn_fd, pthread_self());
  if (!conn) {
	DEBUG_PRINTF("Establishing connection with %d failed", hargs->conn_fd);
	close(hargs->conn_fd);
	return NULL;
  }
  for (;;) {
	if (!conn_handle_incoming_packages_server(hargs->s, conn)) {
	  DEBUG_PRINTF("Connection with %d closed", hargs->conn_fd);
	  return NULL;
	}
	DEBUG_PRINTF("Connection with %d established, commencing normal operations",
				 hargs->conn_fd);
	conn_handle_events_server(conn);
  }
}

typedef struct {
  server *s;
  int socket_fd;
  struct sockaddr_in addr;
} server_listener_args;

_Noreturn static void *
server_listener(void *args) {
  server_listener_args *largs = args;
  server_handler_args *hargs;
  pthread_t h;
  size_t addrlen = sizeof(struct sockaddr_in);
  int conn_fd;
  long iMode = 0;
  for (;;) {
	listen(largs->socket_fd, 3);
	DEBUG_PRINTF("Listening for connections");
	conn_fd = accept(largs->socket_fd, (struct sockaddr *) &largs->addr,
					 (socklen_t *) &addrlen);
	ioctl(conn_fd, FIONBIO, &iMode);
	hargs = malloc(sizeof(server_handler_args));
	hargs->s = largs->s;
	hargs->conn_fd = conn_fd;
	DEBUG_PRINTF("Received connection %d", conn_fd);
	pthread_create(&h, NULL, server_handler, hargs);
  }
}

static void
server_start_conn_listener(server *s, int p) {
  server_listener_args *args;
  int opt = 1;
  DEBUG_PRINTF("Starting connection listener");
  args = malloc(sizeof(server_listener_args));
  args->s = s;
  args->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  setsockopt(args->socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
			 sizeof(opt));
  args->addr.sin_family = AF_INET;
  args->addr.sin_addr.s_addr = INADDR_ANY;
  args->addr.sin_port = htons(p);
  bind(args->socket_fd, (struct sockaddr *) &args->addr, sizeof(args->addr));
  pthread_create(&s->conn_listener, NULL, server_listener, args);
}

void
server_init(server *s, int port) {
  pthread_mutex_init(&s->lock, NULL);
  s->ncons = 0;
  s->port = port;
  skat_state_init(&s->skat_state);
}

static void
server_tick_wrap(void *s) {
  server_tick(s);
}

_Noreturn void
server_run(server *s) {
  ctimer t;

  ctimer_create(&t, s, server_tick_wrap, (1000 * 1000 * 1000) / SERVER_REFRESH_RATE);// 60Hz

  server_acquire_state_lock(s);
  server_start_conn_listener(s, s->port);
  server_release_state_lock(s);

  DEBUG_PRINTF("Running server");

  ctimer_run(&t);
}
