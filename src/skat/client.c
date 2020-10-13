#include "skat/client.h"
#include "skat/connection.h"
#include "skat/util.h"

#if defined(CONSOLE_INPUT) && CONSOLE_INPUT
#include "skat/console_input.h"
#else
#error "Not yet supported, use the console implementation instead"
#endif

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <skat/ctimer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static void
client_close_all_connections(client *c) {
  DEBUG_PRINTF("Closing open connection to server");
  if (close(c->c2s.c.fd) == -1)
	DERROR_PRINTF("Error while closing connection socket to server: %s",
				  strerror(errno));
}

static void
client_prepare_exit(client *c) {
  c->exit = 1;
  client_close_all_connections(c);
}

void
client_acquire_state_lock(client *c) {
  DPRINTF_COND(DEBUG_LOCK, "Acquiring client state lock from thread %ld",
			   pthread_self());
  pthread_mutex_lock(&c->lock);
  DPRINTF_COND(DEBUG_LOCK, "Acquired client state lock from thread %ld",
			   pthread_self());
}

void
client_release_state_lock(client *c) {
  DPRINTF_COND(DEBUG_LOCK, "Releasing client state lock from thread %ld",
			   pthread_self());
  pthread_mutex_unlock(&c->lock);
  DPRINTF_COND(DEBUG_LOCK, "Released client state lock from thread %ld",
			   pthread_self());
}

typedef struct {
  client *c;
  int socket_fd;
  int resume;
} client_conn_args;

static void *
client_conn_action_sender(void *args) {
  connection_c2s *conn = args;

  DEBUG_PRINTF("Starting client action sender thread");

  conn_handle_actions_client(conn);

  return NULL;
}


static void *
client_conn_thread(void *args) {
  connection_c2s *conn;
  client_conn_args *cargs = args;
  pthread_t action_sender;
  conn = establish_connection_client(cargs->c, cargs->socket_fd, pthread_self(),
									 cargs->resume);
  if (!conn) {
	DERROR_PRINTF("Could not establish connection to server, exiting");
	close(cargs->socket_fd);
	exit(EXIT_FAILURE);
  }

  pthread_create(&action_sender, NULL, client_conn_action_sender, conn);

  for (;;) {
	if (!conn_handle_incoming_packages_client(cargs->c, conn)) {
	  goto ret;
	}
  }
ret:
  DERROR_PRINTF("Returning from function it shouldn't be possible to return "
				"from. Damn");
  return NULL;
}

static void
start_client_conn(client *c, const char *host, int p, int resume) {
  /* Obtain address(es) matching host/port */

  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;       /* Allow IPv4 */
  hints.ai_socktype = SOCK_STREAM; /* TCP socket */
  hints.ai_flags = 0;
  hints.ai_protocol = 0; /* Any protocol */

  char port_str[6];
  sprintf(port_str, "%d", p);

  struct addrinfo *result;
  int error = getaddrinfo(host, port_str, &hints, &result);
  if (error != 0) {
	DERROR_PRINTF("Error while getting address info: %s", gai_strerror(error));
	exit(EXIT_FAILURE);
  }

  /* getaddrinfo() returns a list of address structures.
	  Try each address until we successfully connect(2).
	  If socket(2) (or connect(2)) fails, we (close the socket
	  and) try the next address. */

  int socket_fd;
  struct addrinfo *rp;
  for (rp = result; rp != NULL; rp = rp->ai_next) {
	socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
	if (socket_fd == -1) {
	  continue;
	}

	if (connect(socket_fd, rp->ai_addr, rp->ai_addrlen) != -1) {
	  break; /* Success */
	}

	close(socket_fd);
  }

  if (rp == NULL) { /* No address succeeded */
	DERROR_PRINTF("Could not connect to server");
	exit(EXIT_FAILURE);
  }

  freeaddrinfo(result); /* No longer needed */

  DEBUG_PRINTF("Established connection on socket %d", socket_fd);

  client_conn_args *args = malloc(sizeof(client_conn_args));
  args->c = c;
  args->socket_fd = socket_fd;
  args->resume = resume;

  pthread_create(&c->conn_thread, NULL, client_conn_thread, args);
}

_Noreturn static void *
client_exec_async_handler(void *args) {
  client *c = args;

  async_callback acb;
  for (;;) {
	dequeue_async_callback_blocking(&c->acq, &acb);
	acb.do_stuff(acb.data);
  }
}

static void
start_exec_async_thread(client *c) {
  pthread_create(&c->exec_async_handler, NULL, client_exec_async_handler, c);
}

static void
start_io_thread(client *c) {
  pthread_create(&c->io_handler, NULL, handle_console_input, c);
}

void
client_tick(client *c) {
  DPRINTF_COND(DEBUG_TICK, "Client tick");

  client_acquire_state_lock(c);

  event e;
  // event err_ev;
  while (conn_dequeue_event(&c->c2s.c, &e)) {
	if (!skat_client_state_apply(&c->cs, &e, c)) {
	  DEBUG_PRINTF("Received illegal event of type %s from server, rejecting",
				   event_name_table[e.type]);
	  /*
	  err_ev.type = EVENT_ILLEGAL_ACTION;
	  err_ev.answer_to = a.id;
	  copy_player_name(&err_ev.player, &s->ps[i].id);
	  conn_enqueue_event(&s->conns[i].c, &err_ev);
	   */
	}
  }
  skat_client_state_tick(&c->cs, c);

  client_release_state_lock(c);
}

void
client_ready(client *c) {
  action a;

  memset(&a, '\0', sizeof(a));

  a.type = ACTION_READY;
  a.id = -1;
  DTODO_PRINTF("Actually use the action id properly");
  DEBUG_PRINTF("Enqueueing ready action");
  conn_enqueue_action(&c->c2s.c, &a);
}

void
client_play_card(client *c, unsigned int card_index) {
  action a;

  memset(&a, '\0', sizeof(a));

  a.type = ACTION_PLAY_CARD;
  a.id = -1;
  DTODO_PRINTF("Actually use the action id properly");
  if (card_collection_get_card(&c->cs.my_hand, card_index, &a.card)) {
	DERROR_PRINTF("Could not play the given card index %d, out of range",
				  card_index);
	return;
  }

  DEBUG_PRINTF("Enqueueing play card action");
  conn_enqueue_action(&c->c2s.c, &a);
}

void
client_disconnect_connection(client *c, connection_c2s *conn) {
  DERROR_PRINTF("Lost connection to server");
  conn_disable_conn(&conn->c);
  exit(EXIT_FAILURE);
}

void
client_handle_resync(client *c, payload_resync *pl) {
  DEBUG_PRINTF("Resyncing client state");
  c->cs = pl->scs;

  size_t offset = 0;
  for (int i = 0; i < 4; i++) {
	if (c->pls[i])
	  free(c->pls[i]);

	size_t len = pl->player_name_lengths[i];
	if (len > 0) {
	  c->pls[i] = malloc(sizeof(player) + len + 1);
	  c->pls[i]->index = i;
	  c->pls[i]->name.length = len;

	  memcpy(c->pls[i]->name.name, pl->player_names + offset,
			 len * sizeof(char));
	  c->pls[i]->name.name[len] = '\0';
	  offset += len;
	} else {
	  c->pls[i] = NULL;
	}
  }
}

void
client_notify_join(client *c, payload_notify_join *pl_nj) {
  DEBUG_PRINTF("%s has joined the game", pl_nj->pname.name);

  if (c->pls[pl_nj->gupid])
	free(c->pls[pl_nj->gupid]);
  c->pls[pl_nj->gupid] = create_player(pl_nj->gupid, &pl_nj->pname);

  client_skat_state_notify_join(&c->cs, pl_nj);
}

void
client_notify_leave(client *c, payload_notify_leave *pl_nl) {
  DEBUG_PRINTF("%s has left the game", c->pls[pl_nl->gupid]->name.name);

  free(c->pls[pl_nl->gupid]);
  c->pls[pl_nl->gupid] = NULL;

  client_skat_state_notify_leave(&c->cs, pl_nl);
}

static void client_start_interrupt_handler_thread(client *c);

static void *
client_signal_handler(void *args) {
  client *c = args;

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
	client_start_interrupt_handler_thread(c);
	return NULL;
  }

  DEBUG_PRINTF("Received signal %s (%d)", strsignal(sig), sig);

  client_prepare_exit(c);

  DEBUG_PRINTF("Exiting...");
  exit(128 + sig);
}

static void
client_start_interrupt_handler_thread(client *c) {
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

  pthread_create(&c->signal_listener, NULL, client_signal_handler, c);
}

void
client_init(client *c, char *host, int port, char *name) {
  DEBUG_PRINTF("Initializing client '%s' for server '%s:%d'", name, host, port);
  memset(c, '\0', sizeof(client));
  pthread_mutex_init(&c->lock, NULL);
  init_async_callback_queue(&c->acq);
  c->host = host;
  c->port = port;
  c->name = name;
  client_skat_state_init(&c->cs);
  client_start_interrupt_handler_thread(c);
}

static void
client_tick_wrap(void *c) {
  client_tick(c);
}

_Noreturn void
client_run(client *c, int resume) {
  ctimer t;

  ctimer_create(&t, c, client_tick_wrap,
				(1000 * 1000 * 1000) / CLIENT_REFRESH_RATE);// in Hz

  DEBUG_PRINTF("Running client with with connection mode '%s'",
			   resume ? "resume" : "new");
  client_acquire_state_lock(c);
  start_client_conn(c, c->host, c->port, resume);
  start_exec_async_thread(c);
  start_io_thread(c);
  client_release_state_lock(c);

  ctimer_run(&t);
  pause();
  DERROR_PRINTF("TF did we get here");
  __builtin_unreachable();
}
