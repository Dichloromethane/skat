#include "skat/connection.h"
#include "skat/client.h"
#include "skat/package.h"
#include "skat/server.h"
#include "skat/util.h"
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define CH_ASSERT(stmt, c, err, ret_label) \
  do { \
	if (!(stmt)) { \
	  conn_error(c, err); \
	  goto ret_label; \
	} \
  } while (0)

#define ATOMIC_QUEUE_NO_INCLUDE_HEADER
#define TYPE action
#include "atomic_queue.def"
#undef TYPE

#define TYPE event
#include "atomic_queue.def"
#undef TYPE
#undef ATOMIC_QUEUE_NO_INCLUDE_HEADER

#undef CONNECTION_C_HDR
#define CONNECTION_HDR_TO_STRING

#include "skat/connection.h"

static void
send_package(connection *c, package *p) {
  DEBUG_PRINTF("Sending package of type %s with payload size %lu",
			   package_name_table[p->type], p->payload_size);
  send(c->fd, p, sizeof(package), 0);
  if (p->payload_size > 0) {
	send(c->fd, p->payload.v, p->payload_size, 0);
  }
}

static int
retrieve_package(connection *c, package *p) {
  package_clean(p);

  ssize_t res;
  res = read(c->fd, p, sizeof(package));
  if (res < 0 || (size_t) res != sizeof(package)) {
	DERROR_PRINTF("Connection %d unexpectedly terminated while trying to "
				  "retrieve package",
				  c->fd);
	package_clean(p);
	return 0;
  }

  if (p->payload_size > 0) {
	p->payload.v = malloc(p->payload_size);
	res = read(c->fd, p->payload.v, p->payload_size);
	if (res < 0 || (size_t) res != p->payload_size) {
	  DERROR_PRINTF("Connection %d unexpectedly terminated while retrieving "
					"payload of type %s with size %lu",
					c->fd, package_name_table[p->type], p->payload_size);
	  package_free(p);
	  return 0;
	}
  } else {
	p->payload.v = NULL;
  }

  DPRINTF_COND(DEBUG_PACKAGE,
			   "Retrieved package of type %s with payload size %lu",
			   package_name_table[p->type], p->payload_size);
  return 1;
}

static void
conn_error(connection *c, conn_error_type cet) {
  package p;

  package_clean(&p);
  p.type = PACKAGE_ERROR;

  payload_error pl = (payload_error){.type = cet};
  p.payload_size = sizeof(payload_error);
  p.payload.pl_er = &pl;

  DERROR_PRINTF("Connection error: %s", conn_error_name_table[cet]);
  send_package(c, &p);
}

static void
init_conn(connection *c, int fd, pthread_t handler) {
  c->fd = fd;
  c->handler = handler;
  init_action_queue(&c->aq);
  init_event_queue(&c->eq);
  c->active = 0;
}

static void
init_conn_s2c(connection_s2c *s2c, connection *base_conn) {
  s2c->c = *base_conn;
}

static void
init_conn_c2s(connection_c2s *c2s, connection *base_conn) {
  c2s->c = *base_conn;
}

static void
client_disconnect_connection(client *c) {
  DERROR_PRINTF("Lost connection to server");
  client_acquire_state_lock(c);
  client_prepare_exit(c);
  client_release_state_lock(c);
  exit(EXIT_FAILURE);
}

connection_s2c *
establish_connection_server(server *s, int fd, pthread_t handler) {
  __label__ err, err_release;

  package p;
  connection c;
  connection_s2c *s2c;
  int gupid;

  init_conn(&c, fd, handler);

  package_clean(&p);
  retrieve_package(&c, &p);

  // TODO: remove this so we can actually resume a player or add a new player in
  // between rounds
  /* CH_ASSERT(s->ss.sgs.cgphase == GAME_PHASE_SETUP, &c,
			CONN_ERROR_INVALID_JOIN_TIME, err);*/

  if (p.type == PACKAGE_JOIN) {
	payload_join *pl_join = p.payload.pl_j;

	CH_ASSERT(pl_join->network_protocol_version == NETWORK_PROTOCOL_VERSION, &c,
			  CONN_ERROR_PROTOCOL_VERSION_MISMATCH, err);
	CH_ASSERT(pl_join->name_length + 1 < PLAYER_MAX_NAME_LENGTH, &c,
			  CONN_ERROR_NAME_TOO_LONG, err);

	server_acquire_state_lock(s);

	DEBUG_PRINTF("New player join with name '%s'", pl_join->name);
	for (int i = 0; i < 4; i++) {
	  player *pl = s->pls[i];
	  if (pl) {
		int active = server_is_player_active(s, i);
		DEBUG_PRINTF("Existing player %d: '%s' (%s)", i, pl->name,
					 active ? "active" : "inactive");
	  } else {
		DEBUG_PRINTF("Empty player slot %d", i);
	  }
	}

	CH_ASSERT(!server_has_player_name(s, pl_join->name), &c,
			  CONN_ERROR_PLAYER_NAME_IN_USE, err_release);

	CH_ASSERT(s2c = server_get_free_connection(s, &gupid), &c,
			  CONN_ERROR_TOO_MANY_PLAYERS, err_release);

	player *pl = create_player(gupid, -1, pl_join->name);

	init_conn_s2c(s2c, &c);
	s2c->c.active = 1;
	s2c->gupid = gupid;

	server_add_player_for_connection(s, pl, gupid);

	server_notify_join(s, gupid);

	server_release_state_lock(s);

	package_free(&p);

	p.type = PACKAGE_CONFIRM_JOIN;

	payload_confirm_join pl_cj = (payload_confirm_join){.gupid = gupid};
	p.payload_size = sizeof(payload_confirm_join);
	p.payload.pl_cj = &pl_cj;

	send_package(&s2c->c, &p);

	return s2c;
  } else if (p.type == PACKAGE_CONN_RESUME) {
	payload_resume *pl_resume = p.payload.pl_rm;

	CH_ASSERT(pl_resume->network_protocol_version == NETWORK_PROTOCOL_VERSION,
			  &c, CONN_ERROR_PROTOCOL_VERSION_MISMATCH, err);
	CH_ASSERT(pl_resume->name_length + 1 < PLAYER_MAX_NAME_LENGTH, &c,
			  CONN_ERROR_NAME_TOO_LONG, err);

	server_acquire_state_lock(s);

	DEBUG_PRINTF("Resuming player join with name '%s'", pl_resume->name);
	for (int i = 0; i < 4; i++) {
	  player *pl = s->pls[i];
	  if (pl) {
		int active = server_is_player_active(s, i);
		DEBUG_PRINTF("Existing player %d: %s (%s)", i, pl->name,
					 active ? "active" : "inactive");
	  } else {
		DEBUG_PRINTF("Empty player slot %d", i);
	  }
	}

	CH_ASSERT(s2c = server_get_connection_by_pname(s, pl_resume->name, &gupid),
			  &c, CONN_ERROR_NO_SUCH_PLAYER_NAME, err_release);
	CH_ASSERT(!s2c->c.active, &c, CONN_ERROR_PLAYER_NAME_IN_USE, err_release);

	init_conn_s2c(s2c, &c);
	s2c->c.active = 1;

	server_resume_player_for_connection(s, gupid);

	server_notify_join(s, gupid);

	server_release_state_lock(s);

	package_free(&p);

	p.type = PACKAGE_CONFIRM_RESUME;

	payload_confirm_resume pl_res = (payload_confirm_resume){.gupid = gupid};
	p.payload_size = sizeof(payload_confirm_join);
	p.payload.pl_cr = &pl_res;

	send_package(&s2c->c, &p);

	return s2c;
  }

  CH_ASSERT(0, &c, CONN_ERROR_INVALID_CONN_STATE, err);

err_release:
  server_release_state_lock(s);
err:
  return NULL;
}

static int
conn_handle_incoming_package_client_single(client *c, connection_c2s *conn,
										   package *p) {
  __label__ err;

  switch (p->type) {
	case PACKAGE_EVENT:// clients receive events and send actions
	  conn_enqueue_event(&conn->c, &p->payload.pl_ev->ev);
	  break;
	case PACKAGE_CONFIRM_JOIN:
	  __attribute__((fallthrough));
	case PACKAGE_CONFIRM_RESUME:
	  conn->c.active = 1;
	  break;
	case PACKAGE_ERROR:
	  conn->c.active = 0;
	  DERROR_PRINTF("Received error from server, disconnecting");
	  client_disconnect_connection(c);
	  return 0;
	case PACKAGE_NOTIFY_JOIN:
	  client_acquire_state_lock(c);
	  client_notify_join(c, p->payload.pl_nj);
	  client_release_state_lock(c);
	  break;
	case PACKAGE_NOTIFY_LEAVE:
	  client_acquire_state_lock(c);
	  client_notify_leave(c, p->payload.pl_nl);
	  client_release_state_lock(c);
	  break;
	default:
	  DTODO_PRINTF("TODO: rest of the client side protocol");// TODO: this
	  CH_ASSERT(0, &conn->c, CONN_ERROR_INVALID_PACKAGE_TYPE, err);
  }
  return 1;

err:
  return 0;
}

static int
conn_await_package(connection *c, package *p, int (*acceptor)(package *)) {
  do {
	package_free(p);
	if (!retrieve_package(c, p))
	  return 0;
  } while (!acceptor(p));
  return 1;
}

static int
conf_resync_acceptor(package *p) {
  return p->type == PACKAGE_ERROR || p->type == PACKAGE_RESYNC;
}

int
conn_handle_incoming_packages_client(client *c, connection_c2s *conn) {
  package p;
  int still_connected;

  package_clean(&p);
  still_connected = retrieve_package(&conn->c, &p);
  if (!still_connected) {
	client_disconnect_connection(c);
	return 0;
  }

  int result = conn_handle_incoming_package_client_single(c, conn, &p);
  package_free(&p);
  return result;
}

connection_c2s *
establish_connection_client(client *c, int socket_fd, pthread_t handler,
							int resume) {
  __label__ err;

  DEBUG_PRINTF("%s connection to server",
			   resume ? "Resuming" : "Establishing new");

  connection base_conn;
  init_conn(&base_conn, socket_fd, handler);

  connection_c2s *c2s = &c->c2s;
  init_conn_c2s(c2s, &base_conn);

  package p;
  size_t name_length = strlen(c->name);

  package_clean(&p);
  if (resume) {
	p.type = PACKAGE_CONN_RESUME;

	size_t pl_size = sizeof(payload_resume) + name_length + 1;
	payload_resume *pl = malloc(pl_size);
	pl->network_protocol_version = NETWORK_PROTOCOL_VERSION;
	pl->name_length = name_length;
	memcpy(pl->name, c->name, name_length + 1);
	p.payload_size = pl_size;
	p.payload.pl_rm = pl;
  } else {
	p.type = PACKAGE_JOIN;

	size_t pl_size = sizeof(payload_join) + name_length + 1;
	payload_join *pl = malloc(pl_size);
	pl->network_protocol_version = NETWORK_PROTOCOL_VERSION;
	pl->name_length = name_length;
	memcpy(pl->name, c->name, name_length + 1);
	p.payload_size = pl_size;
	p.payload.pl_j = pl;
  }

  send_package(&c2s->c, &p);

  package_free(&p);

  if (!retrieve_package(&c2s->c, &p))
	return NULL;

  if (p.type == PACKAGE_ERROR) {
	DERROR_PRINTF("Encountered error %s while connecting to server",
				  conn_error_name_table[p.payload.pl_er->type]);
	return NULL;
  }

  CH_ASSERT((!resume && p.type == PACKAGE_CONFIRM_JOIN)
					|| (resume && p.type == PACKAGE_CONFIRM_RESUME),
			&c2s->c, CONN_ERROR_INVALID_CONN_STATE, err);

  package_free(&p);

  p.type = PACKAGE_RESYNC;
  send_package(&c2s->c, &p);

  package_clean(&p);

  if (!conn_await_package(&c2s->c, &p, conf_resync_acceptor))
	return NULL;

  if (p.type == PACKAGE_ERROR) {
	DERROR_PRINTF("Encountered error %s while resyncing with server",
				  conn_error_name_table[p.payload.pl_er->type]);
	return NULL;
  }

  client_handle_resync(c, p.payload.pl_rs);

  package_free(&p);

  p.type = PACKAGE_CONFIRM_RESYNC;
  send_package(&c2s->c, &p);

  package_clean(&p);

  return c2s;

err:
  return NULL;
}

int
conf_resync_confirm_acceptor(package *p) {
  return p->type == PACKAGE_CONFIRM_RESYNC;
}

static int
conn_resync_player(server *s, connection_s2c *c) {
  package p;

  package_clean(&p);
  p.type = PACKAGE_RESYNC;

  player *pl = s->pls[c->gupid];

  p.payload_size = server_resync_player(s, pl, &p.payload.pl_rs);

  send_package(&c->c, &p);

  package_free(&p);

  // TODO: deal with PACKAGE_ERROR ?
  int still_connected =
		  conn_await_package(&c->c, &p, conf_resync_confirm_acceptor);
  package_free(&p);
  return still_connected;
}

static int
conn_handle_incoming_packages_server_single(server *s, connection_s2c *c,
											package *p) {
  __label__ err;
  int still_connected;
  payload_action *pl_ac;

  switch (p->type) {
	case PACKAGE_ACTION:// server distributes events and receives actions
	  pl_ac = p->payload.pl_a;
	  conn_enqueue_action(&c->c, &pl_ac->ac);
	  break;
	case PACKAGE_RESYNC:
	  server_acquire_state_lock(s);
	  still_connected = conn_resync_player(s, c);
	  server_release_state_lock(s);
	  return still_connected;
	case PACKAGE_ERROR:
	  DERROR_PRINTF("Received error from client, killing him");
	  __attribute__((fallthrough));
	case PACKAGE_DISCONNECT:
	  server_acquire_state_lock(s);
	  server_disconnect_connection(s, c);
	  server_release_state_lock(s);
	  return 0;
	default:
	  CH_ASSERT(0, &c->c, CONN_ERROR_INVALID_PACKAGE_TYPE, err);
  }
  return 1;

err:
  return 0;
}

int
conn_handle_incoming_packages_server(server *s, connection_s2c *c) {
  package p;
  int still_connected;

  package_clean(&p);
  still_connected = retrieve_package(&c->c, &p);
  if (!still_connected) {
	server_acquire_state_lock(s);
	server_disconnect_connection(s, c);
	server_release_state_lock(s);
	return 0;
  }

  int result = conn_handle_incoming_packages_server_single(s, c, &p);
  package_free(&p);
  return result;
}

_Noreturn void
conn_handle_events_server(connection_s2c *c) {
  payload_event pl_ev;
  package p;

  package_clean(&p);
  p.type = PACKAGE_EVENT;
  p.payload_size = sizeof(payload_event);
  p.payload.pl_ev = &pl_ev;

  for (;;) {
	conn_dequeue_event_blocking(&c->c, &pl_ev.ev);
	DEBUG_PRINTF("Sending new event to server");
	send_package(&c->c, &p);
  }
}

_Noreturn void
conn_handle_actions_client(connection_c2s *conn) {
  payload_action pl_a;
  package p;

  package_clean(&p);
  p.type = PACKAGE_ACTION;
  p.payload_size = sizeof(payload_action);
  p.payload.pl_a = &pl_a;


  for (;;) {
	conn_dequeue_action_blocking(&conn->c, &pl_a.ac);
	DEBUG_PRINTF("Sending new action to server");
	send_package(&conn->c, &p);
  }
}

void
conn_notify_join(connection_s2c *c, player *pl) {
  package p;

  if (!c->c.active)
	return;

  package_clean(&p);
  p.type = PACKAGE_NOTIFY_JOIN;

  size_t pl_nj_size = sizeof(payload_notify_join) + pl->name_length + 1;
  payload_notify_join *pl_nj = malloc(pl_nj_size);

  pl_nj->gupid = pl->gupid;
  pl_nj->ap = pl->ap;

  pl_nj->name_length = pl->name_length;
  memcpy(pl_nj->name, pl->name, pl->name_length + 1);

  p.payload_size = pl_nj_size;
  p.payload.pl_nj = pl_nj;

  send_package(&c->c, &p);

  package_free(&p);
}

void
conn_notify_disconnect(connection_s2c *c, player *pl) {
  package p;

  if (!c->c.active)
	return;

  package_clean(&p);
  p.type = PACKAGE_NOTIFY_LEAVE;

  size_t pl_nl_size = sizeof(payload_notify_leave);
  payload_notify_leave pl_nl;
  memset(&pl_nl, '\0', pl_nl_size);
  pl_nl.gupid = pl->gupid;

  p.payload_size = pl_nl_size;
  p.payload.pl_nl = &pl_nl;

  send_package(&c->c, &p);

  package_clean(&p);
}

void
conn_disable_conn(connection *c) {
  c->active = 0;
  // FIXME: this causes a deadlock
  // clear_action_queue(&c->aq);
  // clear_event_queue(&c->eq);
  if (close(c->fd) == -1)
	DERROR_PRINTF("Error while closing connection : %s", strerror(errno));
}

int
conn_dequeue_action(connection *c, action *a) {
  return dequeue_action(&c->aq, a);
}

void
conn_dequeue_action_blocking(connection *c, action *a) {
  dequeue_action_blocking(&c->aq, a);
}

void
conn_enqueue_event(connection *c, event *e) {
  enqueue_event(&c->eq, e);
}

void
conn_enqueue_action(connection *c, action *a) {
  enqueue_action(&c->aq, a);
}

int
conn_dequeue_event(connection *c, event *e) {
  return dequeue_event(&c->eq, e);
}

void
conn_dequeue_event_blocking(connection *c, event *e) {
  dequeue_event_blocking(&c->eq, e);
}
