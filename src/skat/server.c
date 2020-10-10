#include "skat/server.h"
#include "conf.h"
#include "skat/ctimer.h"
#include "skat/util.h"
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define FOR_EACH_ACTIVE(s, var, block) \
  do { \
	for (int var = 0; var < 4; var++) { \
	  if (!server_is_player_active(s, var)) \
		continue; \
	  else \
		block \
	} \
  } while (0)

int
server_is_player_active(server *s, int gupid) {
  return (s->playermask >> gupid) & 1;
}

int
server_has_player_name(server *s, player_name *pname) {
  for (int i = 0; i < 4; i++)// otherwise we can't recover connections
	if (s->ps[i] != NULL && player_name_equals(&s->ps[i]->name, pname))
	  return 1;
  return 0;
}

void
server_send_event(server *s, event *e, player *pl) {
  connection_s2c *c = server_get_connection_by_gupid(s, pl->index);
  if (c) {
	conn_enqueue_event(&c->c, e);
  }
}

player *
server_get_player_by_gupid(server *s, int gupid) {
  return s->ps[gupid];
}

void
server_distribute_event(server *s, event *ev,
						void (*mask_event)(event *, player *)) {
  event e;
  DEBUG_PRINTF("Distributing event of type %s", event_name_table[ev->type]);
  FOR_EACH_ACTIVE(s, i, {
	if (mask_event) {
	  e = *ev;
	  mask_event(&e, s->ps[i]);
	  server_send_event(s, &e, s->ps[i]);
	} else
	  server_send_event(s, ev, s->ps[i]);
  });
}

connection_s2c *
server_get_free_connection(server *s, int *n) {
  int pm, i;
  if (s->ncons > 4)
	return NULL;
  pm = ~s->playermask;
  i = __builtin_ctz(pm);
  s->playermask |= 1 << i;
  *n = i;
  return &s->conns[i];
}

void
server_add_player_for_connection(server *s, player *pl, int n) {
  s->ps[n] = pl;
  pl->index = n;
  s->ncons++;
}

connection_s2c *
server_get_connection_by_pname(server *s, player_name *pname, int *n) {
  FOR_EACH_ACTIVE(s, i, {
	if (player_name_equals(&s->ps[i]->name, pname)) {
	  if (n)
		*n = i;
	  return &s->conns[i];
	}
  });
  return NULL;
}

connection_s2c *
server_get_connection_by_gupid(server *s, int gupid) {
  return &s->conns[gupid];
}

player *
server_get_player_by_pname(server *s, player_name *pname) {
  for (int i = 0; i < 4; i++)
	if (player_name_equals(&s->ps[i]->name, pname))
	  return s->ps[i];
  return NULL;
}

void
server_disconnect_connection(server *s, connection_s2c *c) {
  player *pl;
  pl = server_get_player_by_gupid(s, c->gupid);

  skat_state_notify_disconnect(&s->ss, pl, s);
  FOR_EACH_ACTIVE(s, i, {
	if (!player_equals_by_name(pl, s->ps[i]))
	  conn_notify_disconnect(&s->conns[i], pl);
  });
  s->ncons--;
  conn_disable_conn(&c->c);
}

void
server_acquire_state_lock(server *s) {
  DPRINTF_COND(DEBUG_LOCK, "Acquiring server state lock from thread %d",
			   gettid());
  pthread_mutex_lock(&s->lock);
  DPRINTF_COND(DEBUG_LOCK, "Acquired server state lock from thread %d",
			   gettid());
}

void
server_release_state_lock(server *s) {
  DPRINTF_COND(DEBUG_LOCK, "Releasing server state lock from thread %d",
			   gettid());
  pthread_mutex_unlock(&s->lock);
  DPRINTF_COND(DEBUG_LOCK, "Released server state lock from thread %d",
			   gettid());
}

void
server_resync_player(server *s, player *pl, skat_client_state *cs) {
  DEBUG_PRINTF("Resync requested by player '%s'", pl->name.name);
  skat_resync_player(&s->ss, cs, pl);
}

void
server_tick(server *s) {
  DPRINTF_COND(DEBUG_TICK, "Server tick");

  server_acquire_state_lock(s);

  if (!s->exit) {
	action a;
	event err_ev;
	FOR_EACH_ACTIVE(s, i, {
	  if (!s->conns[i].c.active)
		continue;

	  while (conn_dequeue_action(&s->conns[i].c, &a)) {
		if (!skat_server_state_apply(&s->ss, &a, s->ps[i], s)) {
		  DEBUG_PRINTF("Received illegal action of type %s from player %s with "
					   "id %ld, rejecting",
					   action_name_table[a.type], s->ps[i]->name.name, a.id);
		  err_ev.type = EVENT_ILLEGAL_ACTION;
		  err_ev.answer_to = a.id;
		  copy_player_name(&err_ev.player, &s->ps[i]->name);
		  conn_enqueue_event(&s->conns[i].c, &err_ev);
		}
	  }
	  skat_server_state_tick(&s->ss, s);
	});
  }

  server_release_state_lock(s);
}

void
server_notify_join(server *s, int gupid) {
  player *pl = server_get_player_by_gupid(s, gupid);
  DEBUG_PRINTF("Player '%s' joined with gupid %d", pl->name.name, gupid);
  skat_state_notify_join(&s->ss, pl, s);
  FOR_EACH_ACTIVE(s, i, {
	if (i != gupid)
	  conn_notify_join(&s->conns[i], pl);
  });
}

typedef struct {
  server *s;
  int conn_fd;
} server_handler_args;

static void *
server_conn_event_sender(void *args) {
  connection_s2c *conn = args;

  DEBUG_PRINTF("Starting server event sender thread");

  conn_handle_events_server(conn);

  return NULL;
}

static void *
server_handler(void *args) {
  connection_s2c *conn;
  server_handler_args *hargs = args;
  pthread_t event_sender;
  DEBUG_PRINTF("Handler started, establishing connection with %d",
			   hargs->conn_fd);
  conn = establish_connection_server(hargs->s, hargs->conn_fd, pthread_self());
  if (!conn) {
	DEBUG_PRINTF("Establishing connection with %d failed", hargs->conn_fd);
	close(hargs->conn_fd);
	return NULL;
  }
  DEBUG_PRINTF("Connection with %d established, commencing normal operations",
			   hargs->conn_fd);

  pthread_create(&event_sender, NULL, server_conn_event_sender, conn);

  for (;;) {
	if (!conn_handle_incoming_packages_server(hargs->s, conn)) {
	  DEBUG_PRINTF("Connection with %d closed", hargs->conn_fd);
	  return NULL;
	}
  }
}

_Noreturn static void *
server_listener(void *args) {
  server *s = args;
  server_listener_args *largs = &s->listener;
  server_handler_args *hargs;
  pthread_t h;
  size_t addrlen = sizeof(struct sockaddr_in);
  int conn_fd;
  long iMode = 0;

  listen(largs->socket_fd, 3);
  DEBUG_PRINTF("Listening for connections");
  for (;;) {
	conn_fd = accept(largs->socket_fd, (struct sockaddr *) &largs->addr,
					 (socklen_t *) &addrlen);
	if (conn_fd == -1) {
	  DERROR_PRINTF("Error while accepting connection: %s", strerror(errno));
	  exit(EXIT_FAILURE);
	}

	ioctl(conn_fd, FIONBIO, &iMode);
	hargs = malloc(sizeof(server_handler_args));
	hargs->s = s;
	hargs->conn_fd = conn_fd;
	DEBUG_PRINTF("Received connection %d", conn_fd);
	pthread_create(&h, NULL, server_handler, hargs);
  }
}

static void
server_start_conn_listener(server *s, int p) {
  // int opt = 1;
  DEBUG_PRINTF("Starting connection listener");

  s->listener.socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  /*setsockopt(s->listener.socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
	&opt, sizeof(opt));*/
  s->listener.addr.sin_family = AF_INET;
  s->listener.addr.sin_addr.s_addr = INADDR_ANY;
  s->listener.addr.sin_port = htons(p);
  if (bind(s->listener.socket_fd, (struct sockaddr *) &s->listener.addr,
		   sizeof(s->listener.addr))
	  == -1) {
	DERROR_PRINTF("Error while binding socket address: %s", strerror(errno));
	exit(EXIT_FAILURE);
  }

  pthread_create(&s->conn_listener, NULL, server_listener, s);
}

static void server_start_interrupt_handler_thread(server *s);

static void *
server_signal_handler(void *args) {
  server *s = args;

  DEBUG_PRINTF("Starting interrupt signal handler thread");

  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGHUP);
  sigaddset(&set, SIGTERM);

  int sig;
  int error = sigwait(&set, &sig);
  if (error) {
	DERROR_PRINTF("Error while waiting for interrupt signal: %s",
				  strerror(error));
	exit(EXIT_FAILURE);
  }

  if (!sigismember(&set, sig)) {
	DERROR_PRINTF("Ignoring unexpected signal %s (%d)", strsignal(sig), sig);
	server_start_interrupt_handler_thread(s);
	return NULL;
  }

  DEBUG_PRINTF("Received signal %s (%d)", strsignal(sig), sig);

  server_acquire_state_lock(s);
  s->exit = 1;

  DEBUG_PRINTF("Closing open connections and socket");
  FOR_EACH_ACTIVE(s, i, { close(s->conns[i].c.fd); });
  close(s->listener.socket_fd);

  server_release_state_lock(s);

  DEBUG_PRINTF("Exiting...");
  exit(128 + sig);
}

static void
server_start_interrupt_handler_thread(server *s) {
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGHUP);
  sigaddset(&set, SIGTERM);

  int error = pthread_sigmask(SIG_BLOCK, &set, NULL);
  if (error) {
	DERROR_PRINTF("Could not set signal mask in main thread: %s",
				  strerror(error));
  }

  pthread_create(&s->signal_listener, NULL, server_signal_handler, s);
}

void
server_init(server *s, int port) {
  DEBUG_PRINTF("Initializing server on port '%d'", port);
  memset(s, '\0', sizeof(server));
  pthread_mutex_init(&s->lock, NULL);
  s->port = port;
  server_start_interrupt_handler_thread(s);
  skat_state_init(&s->ss);
}

static void
server_tick_wrap(void *s) {
  server_tick(s);
}

_Noreturn void
server_run(server *s) {
  ctimer_create(&s->tick_timer, s, server_tick_wrap,
				(1000 * 1000 * 1000) / SERVER_REFRESH_RATE);// in Hz

  server_acquire_state_lock(s);
  server_start_conn_listener(s, s->port);
  server_release_state_lock(s);

  DEBUG_PRINTF("Running server");

  ctimer_run(&s->tick_timer);

  pause();
  DERROR_PRINTF("How did we get here? This is illegal");
  __builtin_unreachable();
}
