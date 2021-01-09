#include "skat/connection.h"
#include "skat/byte_buf.h"
#include "skat/client.h"
#include "skat/package.h"
#include "skat/server.h"
#include "skat/util.h"
#include <arpa/inet.h>
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
  byte_buf bb;
  byte_buf_create(&bb);

  package_write(p, &bb);

  uint32_t len = bb.bytes_used;

  if (DEBUG_PACKAGE) {
	str_buf buf;
	str_buf_new_size(&buf, 128);
	byte_buf_dump(&bb, &buf);
	DPRINTF_COND(DEBUG_PACKAGE, "Send package: %s", buf.buf);
	str_buf_free(&buf);
  }

  len = htonl(len);
  ssize_t res = send(c->fd, &len, sizeof(uint32_t), 0);
  if (res < 0 || (size_t) res != sizeof(uint32_t)) {
	DERROR_PRINTF("Connection %d unexpectedly terminated while trying to send "
				  "next package length",
				  c->fd);
	byte_buf_free(&bb);
	return;
  }

  res = send(c->fd, bb.buf, bb.bytes_used, 0);
  if (res < 0 || (size_t) res != bb.bytes_used) {
	DERROR_PRINTF("Connection %d unexpectedly terminated while trying to send "
				  "byte buf of size %zu",
				  c->fd, bb.bytes_used);
	byte_buf_free(&bb);
	return;
  }

  byte_buf_free(&bb);
}

static int
retrieve_package(connection *c, package *p) {
  uint32_t len;
  ssize_t res = read(c->fd, &len, sizeof(uint32_t));
  if (res < 0 || (size_t) res != sizeof(uint32_t)) {
	DERROR_PRINTF(
			"Connection %d unexpectedly terminated while trying to retrieve "
			"next package length. Expected %zu bytes, but got %zd bytes.",
			c->fd, sizeof(uint32_t), res);
	return 0;
  }

  len = ntohl(len);
  if (len == 0) {
	DERROR_PRINTF("Connection %d unexpectedly yielded zero while trying to "
				  "retrieve next package length",
				  c->fd);
	return 0;
  }

  byte_buf bb;
  byte_buf_create_size(&bb, len);
  res = read(c->fd, bb.buf, len);
  if (res < 0 || (size_t) res != len) {
	DERROR_PRINTF("Connection %d unexpectedly terminated while retrieving "
				  "byte buf. Expected %u bytes but got %zd bytes.",
				  c->fd, len, res);
	return 0;
  }
  bb.bytes_used = len;

  if (DEBUG_PACKAGE) {
	str_buf buf;
	str_buf_new_size(&buf, 128);
	byte_buf_dump(&bb, &buf);
	DPRINTF_COND(DEBUG_PACKAGE, "Receive package: %s", buf.buf);
	str_buf_free(&buf);
  }

  package_read(p, &bb);

  byte_buf_free(&bb);
  return 1;
}

static void
conn_error(connection *c, conn_error_type cet) {
  package p;

  package_clean(&p);
  p.type = PACKAGE_ERROR;

  p.pl_er.type = cet;

  DERROR_PRINTF("Connection error: %s", conn_error_name_table[cet]);
  send_package(c, &p);

  package_free(&p);
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
  package p;
  connection c;
  connection_s2c *s2c;
  int8_t gupid;

  init_conn(&c, fd, handler);

  package_clean(&p);
  retrieve_package(&c, &p);

  if (p.type == PACKAGE_JOIN) {
	payload_join *pl_join = &p.pl_j;

	CH_ASSERT(pl_join->network_protocol_version == NETWORK_PROTOCOL_VERSION, &c,
			  CONN_ERROR_PROTOCOL_VERSION_MISMATCH, err);
	CH_ASSERT(strnlen(pl_join->name, PLAYER_MAX_NAME_LENGTH) + 1
					  < PLAYER_MAX_NAME_LENGTH,
			  &c, CONN_ERROR_NAME_TOO_LONG, err);

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

	p.pl_cj.gupid = gupid;

	send_package(&s2c->c, &p);

    package_free(&p);

	return s2c;
  } else if (p.type == PACKAGE_RESUME) {
	payload_resume *pl_resume = &p.pl_rm;

	CH_ASSERT(pl_resume->network_protocol_version == NETWORK_PROTOCOL_VERSION,
			  &c, CONN_ERROR_PROTOCOL_VERSION_MISMATCH, err);
	CH_ASSERT(strnlen(pl_resume->name, PLAYER_MAX_NAME_LENGTH) + 1
					  < PLAYER_MAX_NAME_LENGTH,
			  &c, CONN_ERROR_NAME_TOO_LONG, err);

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

	p.pl_cr.gupid = gupid;

	send_package(&s2c->c, &p);

    package_free(&p);

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
	  conn_enqueue_event(&conn->c, &p->pl_ev.ev);
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
	  client_notify_join(c, &p->pl_nj);
	  client_release_state_lock(c);
	  break;
	case PACKAGE_NOTIFY_LEAVE:
	  client_acquire_state_lock(c);
	  client_notify_leave(c, &p->pl_nl);
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

  package_clean(&p);
  if (resume) {
	p.type = PACKAGE_RESUME;

	p.pl_rm.network_protocol_version = NETWORK_PROTOCOL_VERSION;
	p.pl_rm.name = strdup(c->name);
  } else {
	p.type = PACKAGE_JOIN;

	p.pl_j.network_protocol_version = NETWORK_PROTOCOL_VERSION;
	p.pl_j.name = strdup(c->name);
  }

  send_package(&c2s->c, &p);

  package_free(&p);

  if (!retrieve_package(&c2s->c, &p))
	return NULL;

  if (p.type == PACKAGE_ERROR) {
	DERROR_PRINTF("Encountered error %s while connecting to server",
				  conn_error_name_table[p.pl_er.type]);
	printf("\nConnection error: %s\n", conn_error_name_table[p.pl_er.type]);
	return NULL;
  }

  CH_ASSERT((!resume && p.type == PACKAGE_CONFIRM_JOIN)
					|| (resume && p.type == PACKAGE_CONFIRM_RESUME),
			&c2s->c, CONN_ERROR_INVALID_CONN_STATE, err);

  package_free(&p);

  p.type = PACKAGE_REQUEST_RESYNC;
  send_package(&c2s->c, &p);

  // package free()ed inside conn_await_package(...)
  if (!conn_await_package(&c2s->c, &p, conf_resync_acceptor))
	return NULL;

  if (p.type == PACKAGE_ERROR) {
	DERROR_PRINTF("Encountered error %s while resyncing with server",
				  conn_error_name_table[p.pl_er.type]);
	printf("\nConnection error while resyncing: %s\n",
		   conn_error_name_table[p.pl_er.type]);
	return NULL;
  }

  client_handle_resync(c, &p.pl_rs);

  package_free(&p);

  p.type = PACKAGE_CONFIRM_RESYNC;
  send_package(&c2s->c, &p);

  package_free(&p);

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

  server_resync_player(s, pl, &p.pl_rs);

  send_package(&c->c, &p);

  // package free()ed inside conn_await_package(...)
  // TODO: deal with PACKAGE_ERROR ?
  int still_connected =
		  conn_await_package(&c->c, &p, conf_resync_confirm_acceptor);
  package_free(&p);
  return still_connected;
}

static int
conn_handle_incoming_packages_server_single(server *s, connection_s2c *c,
											package *p) {
  int still_connected;

  switch (p->type) {
	case PACKAGE_ACTION:// server distributes events and receives actions
	  conn_enqueue_action(&c->c, &p->pl_a.ac);
	  break;
	case PACKAGE_REQUEST_RESYNC:
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
  package p;

  package_clean(&p);

  for (;;) {
	p.type = PACKAGE_EVENT;
	conn_dequeue_event_blocking(&c->c, &p.pl_ev.ev);
	DEBUG_PRINTF("Sending new event to server");
	send_package(&c->c, &p);
	package_free(&p);
  }
}

_Noreturn void
conn_handle_actions_client(connection_c2s *conn) {
  package p;

  package_clean(&p);

  for (;;) {
	p.type = PACKAGE_ACTION;
	conn_dequeue_action_blocking(&conn->c, &p.pl_a.ac);
	DEBUG_PRINTF("Sending new action to server");
	send_package(&conn->c, &p);
	package_free(&p);
  }
}

void
conn_notify_join(connection_s2c *c, player *pl) {
  package p;

  if (!c->c.active)
	return;

  package_clean(&p);
  p.type = PACKAGE_NOTIFY_JOIN;

  p.pl_nj.gupid = pl->gupid;
  p.pl_nj.ap = pl->ap;
  p.pl_nj.name = strdup(pl->name);

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

  p.pl_nl.gupid = pl->gupid;

  send_package(&c->c, &p);

  package_free(&p);
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
