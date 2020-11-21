#include "skat/server.h"
#include "conf.h"
#include "skat/ctimer.h"
#include "skat/util.h"
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
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

static void
server_close_all_connections(server *s) {
  DEBUG_PRINTF(
		  "Closing open connection sockets to clients and listener socket");
  FOR_EACH_ACTIVE(s, i, {
	DEBUG_PRINTF("Closing connection socket with id %d", i);
	if (close(s->conns[i].c.fd) == -1)
	  DERROR_PRINTF("Error while closing connection socket to client %d: %s", i,
					strerror(errno));
  });

  DEBUG_PRINTF("Closing listener socket");
  if (close(s->listener.socket_fd) == -1)
	DERROR_PRINTF("Error while closing listener socket: %s", strerror(errno));
}

static void
server_prepare_exit(server *s) {
  s->exit = 1;
  server_close_all_connections(s);
}

int
server_is_player_active(server *s, int gupid) {
  return (s->playermask >> gupid) & 1;
}

int
server_has_player_name(server *s, char *pname) {
  for (int i = 0; i < 4; i++)// otherwise we can't recover connections
	if (s->pls[i] != NULL
		&& !strncmp(s->pls[i]->name, pname, s->pls[i]->name_length))
	  return 1;
  return 0;
}

void
server_send_event(server *s, event *e, player *pl) {
  connection_s2c *c = server_get_connection_by_gupid(s, pl->gupid);
  if (c) {
	conn_enqueue_event(&c->c, e);
  }
}

void
server_distribute_event(server *s, event *ev,
						void (*mask_event)(event *, player *)) {
  event e;
  DEBUG_PRINTF("Distributing event of type %s", event_name_table[ev->type]);
  FOR_EACH_ACTIVE(s, i, {
	if (mask_event) {
	  e = *ev;
	  mask_event(&e, s->pls[i]);
	  server_send_event(s, &e, s->pls[i]);
	} else
	  server_send_event(s, ev, s->pls[i]);
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
server_add_player_for_connection(server *s, player *pl, int gupid) {
  if (s->pls[gupid])
	free(s->pls[gupid]);
  s->pls[gupid] = pl;
  pl->gupid = gupid;
  s->ncons++;
}

connection_s2c *
server_get_connection_by_pname(server *s, char *pname, int *n) {
  FOR_EACH_ACTIVE(s, i, {
	if (!strncmp(s->pls[i]->name, pname, s->pls[i]->name_length)) {
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

void
server_disconnect_connection(server *s, connection_s2c *c) {
  player *pl;
  pl = s->pls[c->gupid];

  DEBUG_PRINTF("Lost connection to client %s (%d)", pl->name, c->gupid);

  skat_state_notify_disconnect(&s->ss, pl, s);
  FOR_EACH_ACTIVE(s, i, {
	if (!player_equals_by_name(pl, s->pls[i]))
	  conn_notify_disconnect(&s->conns[i], pl);
  });
  s->ncons--;
  s->playermask &= ~(1 << c->gupid);
  conn_disable_conn(&c->c);
}

void
server_acquire_state_lock(server *s) {
  char thread_name_buf[THREAD_NAME_SIZE];
  thread_get_name_self(thread_name_buf);

  DPRINTF_COND(DEBUG_LOCK, "Acquiring server state lock from thread '%s'",
			   thread_name_buf);
  pthread_mutex_lock(&s->lock);
  DPRINTF_COND(DEBUG_LOCK, "Acquired server state lock from thread '%s'",
			   thread_name_buf);
}

void
server_release_state_lock(server *s) {
  char thread_name_buf[THREAD_NAME_SIZE];
  thread_get_name_self(thread_name_buf);

  DPRINTF_COND(DEBUG_LOCK, "Releasing server state lock from thread '%s'",
			   thread_name_buf);
  pthread_mutex_unlock(&s->lock);
  DPRINTF_COND(DEBUG_LOCK, "Released server state lock from thread '%s'",
			   thread_name_buf);
}

size_t
server_resync_player(server *s, player *pl, payload_resync **pl_rs) {
  DEBUG_PRINTF("Resync requested by player '%s'", pl->name);

  int active_player_indices[4];
  size_t player_name_lengths[4];
  size_t player_names_length = 0;

  for (int i = 0; i < 4; i++) {
	if (server_is_player_active(s, i)) {
	  active_player_indices[i] = s->pls[i]->ap;
	  player_names_length += (player_name_lengths[i] = s->pls[i]->name_length);
	} else {
	  active_player_indices[i] = -1;
	  player_name_lengths[i] = 0;
	}
  }

  size_t payload_size =
		  sizeof(payload_resync) + player_names_length * sizeof(char);
  *pl_rs = malloc(payload_size);

  skat_resync_player(&s->ss, &(*pl_rs)->scs, pl);

  memcpy((*pl_rs)->active_player_indices, active_player_indices,
		 sizeof(active_player_indices));
  memcpy((*pl_rs)->player_name_lengths, player_name_lengths,
		 sizeof(player_name_lengths));
  size_t offset = 0;
  for (int i = 0; i < 4; i++) {
	if (player_name_lengths[i] > 0) {
	  memcpy((*pl_rs)->player_names + offset, s->pls[i]->name,
			 player_name_lengths[i] * sizeof(char));
	  offset += player_name_lengths[i];
	}
  }

  return payload_size;
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
		if (!skat_server_state_apply(&s->ss, &a, s->pls[i], s)) {
		  DEBUG_PRINTF("Received illegal action of type %s from player %s with "
					   "id %ld, rejecting",
					   action_name_table[a.type], s->pls[i]->name, a.id);
		  err_ev.type = EVENT_ILLEGAL_ACTION;
		  err_ev.answer_to = a.id;
		  err_ev.acting_player = i;
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
  player *pl = s->pls[gupid];
  DEBUG_PRINTF("Player '%s' joined with gupid %d", pl->name, gupid);
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
  thread_set_name(event_sender, "sv_evsdr %d", hargs->conn_fd);

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
  pthread_t handler_thread;

  struct sockaddr_in addr;
  socklen_t addr_len = sizeof(struct sockaddr_in);

  int conn_fd;

  listen(largs->socket_fd, 0);
  DEBUG_PRINTF("Listening for connections");
  for (;;) {
	conn_fd = accept(largs->socket_fd, (struct sockaddr *) &addr, &addr_len);
	if (conn_fd == -1) {
	  DERROR_PRINTF("Error while accepting connection: %s", strerror(errno));
	  server_prepare_exit(s);
	  exit(EXIT_FAILURE);
	}

	hargs = malloc(sizeof(server_handler_args));
	hargs->s = s;
	hargs->conn_fd = conn_fd;
	DEBUG_PRINTF("Received connection %d", conn_fd);

	pthread_create(&handler_thread, NULL, server_handler, hargs);
	thread_set_name(handler_thread, "svcn_hdlr %d", hargs->conn_fd);
  }
}

static void
server_start_conn_listener(server *s, int p) {
  DEBUG_PRINTF("Starting connection listener");

  s->listener.socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  s->listener.addr.sin_family = AF_INET;
  s->listener.addr.sin_addr.s_addr = INADDR_ANY;
  s->listener.addr.sin_port = htons(p);

  int reuse_addr = 1;
  if (setsockopt(s->listener.socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr,
				 sizeof(reuse_addr))
	  == -1) {
	DERROR_PRINTF("Could not set SO_REUSEADDR on listener socket: %s",
				  strerror(errno));
	server_prepare_exit(s);
	exit(EXIT_FAILURE);
  }

  if (bind(s->listener.socket_fd, (struct sockaddr *) &s->listener.addr,
		   sizeof(s->listener.addr))
	  == -1) {
	DERROR_PRINTF("Error while binding socket address: %s", strerror(errno));
	server_prepare_exit(s);
	exit(EXIT_FAILURE);
  }

  pthread_create(&s->conn_listener, NULL, server_listener, s);
  thread_set_name(s->conn_listener, "sv_lstnr");
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
  sigaddset(&set, SIGQUIT);

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

  server_prepare_exit(s);

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
  sigaddset(&set, SIGQUIT);

  int error = pthread_sigmask(SIG_BLOCK, &set, NULL);
  if (error) {
	DERROR_PRINTF("Could not set signal mask in main thread: %s",
				  strerror(error));
  }

  pthread_create(&s->signal_listener, NULL, server_signal_handler, s);
  thread_set_name(s->signal_listener, "sv_sig_lstnr");
}

void
server_init(server *s, int port) {
  DEBUG_PRINTF("Initializing server on port '%d'", port);
  memset(s, '\0', sizeof(server));
  pthread_mutex_init(&s->lock, NULL);
  s->port = port;
  thread_set_name_self("sv_main");
  server_skat_state_init(&s->ss);
  server_start_interrupt_handler_thread(s);
}

static void
server_tick_wrap(void *s) {
  server_tick(s);
}

_Noreturn void
server_run(server *s) {
  long nsecs = (1000L * 1000L * 1000L) / SERVER_REFRESH_RATE;
  ctimer_create(&s->tick_timer, "sv_tick", s, server_tick_wrap, nsecs);

  server_acquire_state_lock(s);
  server_start_conn_listener(s, s->port);
  server_release_state_lock(s);

  DEBUG_PRINTF("Running server");

  ctimer_run(&s->tick_timer);

  pause();
  DERROR_PRINTF("How did we get here? This is illegal");
  __builtin_unreachable();
}
