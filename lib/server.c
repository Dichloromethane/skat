
#include "server.h"
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

int
		server_has_player_id(server *s, player_id pid) {
  for (int i = 0; i < s->ncons; i++)
	if (player_id_equals(&s->ps[i].id, &pid))
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
  connection_s2c *conns = malloc(sizeof(connection_s2c) * (s->ncons + 1));
  player *ps = malloc(sizeof(player) * (s->ncons + 1));
  if (s->conns)
	memcpy(conns, s->conns, sizeof(connection_s2c) * s->ncons);
  if (s->ps)
	memcpy(ps, s->ps, sizeof(connection_s2c) * s->ncons);
  free(s->conns);
  free(s->ps);
  s->conns = conns;
  s->ps = ps;
  return &s->conns[s->ncons++];
}

void
		server_add_player(server *s, player *pl) {
  server_get_free_connection(s);
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
  pthread_mutex_lock(&s->lock);
}

void
		server_release_state_lock(server *s) {
  pthread_mutex_unlock(&s->lock);
}

void
		server_resync_player(server *s, player *pl, skat_client_state *cs) {
  skat_resync_player(cs, pl);
}

void
		server_tick(server *s, long time) {
  action a;
  event err_ev;
  server_acquire_state_lock(s);
  for (int i = 0; i < s->ncons; i++) {
	if (!s->conns[i].active)
	  continue;

	while (conn_dequeue_action(&s->conns[i], &a)) {
	  if (!skat_state_apply(&s->skat_state, &a, &s->ps[i], s)) {
		err_ev.type = EVENT_ILLEGAL_ACTION;
		err_ev.answer_to = a.id;
		memcpy(&err_ev.player, &s->ps[i].id, PLAYER_ID_LENGTH);
		conn_enqueue_event(&s->conns[i], &err_ev);
	  }
	}
	skat_state_tick(&s->skat_state, s);
  }
  server_release_state_lock(s);
}

void
		server_notify_join(server *s, player *pl) {
  skat_state_notify_join(&s->skat_state, pl, s);
  for (int i = 0; i < s->ncons; i++)
	if (!player_equals_by_id(pl, &s->ps[i]))
	  conn_notify_join(&s->conns[i], pl);
}

typedef struct {
  server *s;
  int conn_fd;
} handler_args;

static void *
		handler(void *args) {
  connection_s2c *conn;
  handler_args *hargs = args;
  conn = establish_connection_server(hargs->s, hargs->conn_fd, pthread_self());
  if (!conn) {
	close(hargs->conn_fd);
	return NULL;
  }
  for (;;) {
	if (!conn_handle_incoming_packages_server(hargs->s, conn)) {
	  return NULL;
	}
	conn_handle_events_server(conn);
  }
}

typedef struct {
  server *s;
  int socket_fd;
  struct sockaddr_in addr;
} listener_args;

static void *
		listener(void *args) {
  listener_args *largs = args;
  handler_args *hargs;
  pthread_t h;
  size_t addrlen = sizeof(struct sockaddr_in);
  int conn_fd;
  long iMode = 0;
  for (;;) {
	listen(largs->socket_fd, 3);
	conn_fd = accept(largs->socket_fd, (struct sockaddr *) &largs->addr,
					 (socklen_t *) &addrlen);
	ioctl(conn_fd, FIONBIO, &iMode);
	hargs = malloc(sizeof(handler_args));
	hargs->s = largs->s;
	hargs->conn_fd = conn_fd;

	pthread_create(&h, NULL, handler, hargs);
  }
  return NULL;
}

static void
		start_conn_listener(server *s, int p) {
  listener_args *args;
  int opt = 1;
  args = malloc(sizeof(listener_args));
  args->s = s;
  args->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  setsockopt(args->socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
			 sizeof(opt));
  args->addr.sin_family = AF_INET;
  args->addr.sin_addr.s_addr = INADDR_ANY;
  args->addr.sin_port = htons(p);
  bind(args->socket_fd, (struct sockaddr *) &args->addr, sizeof(args->addr));
  pthread_create(&s->conn_listener, NULL, listener, args);
}
