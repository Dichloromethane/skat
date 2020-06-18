#include "skat/connection.h"
#include "skat/client.h"
#include "skat/package.h"
#include "skat/server.h"
#include "skat/util.h"
#include <stddef.h>
#include <sys/socket.h>
#include <unistd.h>

#define CH_ASSERT(stmt, c, err, ret) \
  do { \
	if (!(stmt)) { \
	  conn_error(c, err); \
	  return ret; \
	} \
  } while (0)

#define CH_ASSERT_1(stmt, c, err)    CH_ASSERT(stmt, c, err, 1)
#define CH_ASSERT_0(stmt, c, err)    CH_ASSERT(stmt, c, err, 0)
#define CH_ASSERT_NULL(stmt, c, err) CH_ASSERT(stmt, c, err, NULL)

#undef CONNECTION_C_HDR
#define CONNECTION_HDR_TO_STRING

#include "skat/connection.h"

static void
send_package(connection *c, package *p) {
  DEBUG_PRINTF("Sending package of type %s with payload size %lu",
			   package_name_table[p->type], p->payload_size);
  send(c->fd, p, sizeof(package), 0);
  if (p->payload_size > 0) {
	send(c->fd, p->payload, p->payload_size, 0);
  }
}

static int
retrieve_package(connection *c, package *p) {
  ssize_t res;
  res = read(c->fd, p, sizeof(package));
  if (res < 0 || (size_t) res != sizeof(package)) {
	DERROR_PRINTF("Connection %d unexpectedly terminated while trying to "
				  "retrieve package",
				  c->fd);
	return 0;
  }

  if (p->payload_size > 0) {
	p->payload = malloc(p->payload_size);
	res = read(c->fd, p->payload, p->payload_size);
	if (res < 0 || (size_t) res != p->payload_size) {
	  DERROR_PRINTF("Connection %d unexpectedly terminated while retrieving "
					"payload of type %s with size %lu",
					c->fd, package_name_table[p->type], p->payload_size);
	  free(p->payload);
	  p->payload = NULL;
	  return 0;
	}
  } else {
	p->payload = NULL;
  }

  DEBUG_PRINTF("Retrieved package of type %s with payload size %lu",
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
  p.payload = &pl;

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
  init_player_id(&s2c->pid, "");
}

static void
init_conn_c2s(connection_c2s *c2s, connection *base_conn) {
  c2s->c = *base_conn;
}

connection_s2c *
establish_connection_server(server *s, int fd, pthread_t handler) {
  package p;
  connection c;
  connection_s2c *s2c;
  player pl;
  int n;

  init_conn(&c, fd, handler);

  package_clean(&p);
  retrieve_package(&c, &p);

  if (p.type == PACKAGE_JOIN) {
	payload_join *pl_join = p.payload;
	copy_player_id(&pl.id, &pl_join->pid);

	server_acquire_state_lock(s);

	CH_ASSERT_NULL(!server_has_player_id(s, &pl.id), &c,
				   CONN_ERROR_PLAYER_ID_IN_USE);

	CH_ASSERT_NULL(s2c = server_get_free_connection(s, &n), &c,
				   CONN_ERROR_TOO_MANY_PLAYERS);

	init_conn_s2c(s2c, &c);
	s2c->c.active = 1;
	s2c->pid = pl.id;
	pl.index = n;

	server_add_player_for_connection(s, &pl, n);

	server_notify_join(s, &pl);

	server_release_state_lock(s);

	package_free(&p);

	p.type = PACKAGE_CONFIRM_JOIN;

	payload_confirm_join pl_cj = (payload_confirm_join){.player_index = n};
	p.payload_size = sizeof(payload_confirm_join);
	p.payload = &pl_cj;

	send_package(&s2c->c, &p);

	return s2c;
  }
  if (p.type == PACKAGE_CONN_RESUME) {
	payload_resume *pl_resume = p.payload;
	copy_player_id(&pl.id, &pl_resume->pid);

	server_acquire_state_lock(s);

	CH_ASSERT_NULL(s2c = server_get_connection_by_pid(s, pl.id, &n), &c,
				   CONN_ERROR_NO_SUCH_PLAYER_ID);
	CH_ASSERT_NULL(!s2c->c.active, &s2c->c, CONN_ERROR_PLAYER_ID_IN_USE);

	init_conn_s2c(s2c, &c);
	s2c->c.active = 1;
	s2c->pid = pl.id;
	pl.index = n;

	server_notify_join(s, &pl);

	server_release_state_lock(s);

	package_free(&p);

	p.type = PACKAGE_CONFIRM_RESUME;

	payload_confirm_resume pl_res = (payload_confirm_resume){.player_index = n};
	p.payload_size = sizeof(payload_confirm_join);
	p.payload = &pl_res;

	send_package(&s2c->c, &p);

	return s2c;
  }
  CH_ASSERT_NULL(0, &c, CONN_ERROR_INVALID_CONN_STATE);
}

static int
conn_handle_incoming_package_client_single(client *c, connection_c2s *conn,
										   package *p) {
  payload_event *pl_ev;
  switch (p->type) {
	case PACKAGE_EVENT:// clients receive events and send actions
	  pl_ev = p->payload;
	  conn_enqueue_event(&conn->c, &pl_ev->ev);
	  break;
	case PACKAGE_CONFIRM_JOIN:
	  __attribute__((fallthrough));
	case PACKAGE_CONFIRM_RESUME:
	  conn->c.active = 1;
	  break;
	case PACKAGE_ERROR:
	  conn->c.active = 0;
	  DERROR_PRINTF("Received error from server, disconnecting");
	  client_acquire_state_lock(c);
	  client_disconnect_connection(c, conn);
	  client_release_state_lock(c);
	  return 0;
	default:
	  DTODO_PRINTF("TODO: rest of the client side protocol");// TODO: this
	  CH_ASSERT_0(0, &conn->c, CONN_ERROR_INVALID_PACKAGE_TYPE);
  }
  return 1;
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
  return p->type == PACKAGE_RESYNC;
}

int
conn_handle_incoming_packages_client(client *c, connection_c2s *conn) {
  package p;
  int still_connected;

  package_clean(&p);
  still_connected = retrieve_package(&conn->c, &p);
  if (!still_connected) {
	client_acquire_state_lock(c);
	client_disconnect_connection(c, conn);
	client_release_state_lock(c);
	return 0;
  }

  int result = conn_handle_incoming_package_client_single(c, conn, &p);
  package_free(&p);
  return result;
}

connection_c2s *
establish_connection_client(client *c, int socket_fd, pthread_t handler,
							int resume) {
  DEBUG_PRINTF("%s connection to server",
			   resume ? "Resuming" : "Establishing new");

  connection base_conn;
  init_conn(&base_conn, socket_fd, handler);

  connection_c2s *c2s = &c->c2s;
  init_conn_c2s(c2s, &base_conn);

  package p;
  player_id pid;
  init_player_id(&pid, c->name);

  package_clean(&p);
  if (resume) {
	p.type = PACKAGE_CONN_RESUME;

	payload_resume pl = (payload_resume){.pid = pid};
	p.payload_size = sizeof(payload_resume);
	p.payload = &pl;
  } else {
	p.type = PACKAGE_JOIN;

	payload_join pl = (payload_join){.pid = pid};
	p.payload_size = sizeof(payload_join);
	p.payload = &pl;
  }

  send_package(&c2s->c, &p);

  package_clean(&p);

  if (!retrieve_package(&c2s->c, &p))
	return NULL;

  CH_ASSERT_NULL((!resume && p.type == PACKAGE_CONFIRM_JOIN)
						 || (resume && p.type == PACKAGE_CONFIRM_RESUME),
				 &c2s->c, CONN_ERROR_INVALID_CONN_STATE);

  package_free(&p);

  p.type = PACKAGE_RESYNC;
  send_package(&c2s->c, &p);

  package_clean(&p);

  if (!conn_await_package(&c2s->c, &p, conf_resync_acceptor))
	return NULL;

  client_handle_resync(c, p.payload);

  package_free(&p);

  p.type = PACKAGE_CONFIRM_RESYNC;
  send_package(&c2s->c, &p);

  package_clean(&p);

  return c2s;
}

int
conf_resync_confirm_acceptor(package *p) {
  return p->type == PACKAGE_CONFIRM_RESYNC;
}

static int
conn_resync_player(server *s, connection_s2c *c, package *req_p) {
  package p;

  package_clean(&p);
  p.type = PACKAGE_RESYNC;

  player *pl = server_get_player_by_pid(s, c->pid);

  payload_resync pl_rs;
  server_resync_player(s, pl, &pl_rs.scs);

  p.payload_size = sizeof(payload_resync);
  p.payload = &pl_rs;

  send_package(&c->c, &p);

  package_clean(&p);

  return conn_await_package(&c->c, req_p, conf_resync_confirm_acceptor);
}

static int
conn_handle_incoming_packages_server_single(server *s, connection_s2c *c,
											package *p) {
  int still_connected;
  payload_action *pl_ac;

  switch (p->type) {
	case PACKAGE_ACTION:// server distributes events and receives actions
	  pl_ac = p->payload;
	  conn_enqueue_action(&c->c, &pl_ac->ac);
	  break;
	case PACKAGE_RESYNC:
	  server_acquire_state_lock(s);
	  still_connected = conn_resync_player(s, c, p);
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
	  CH_ASSERT_0(0, &c->c, CONN_ERROR_INVALID_PACKAGE_TYPE);
  }
  return 1;
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

void
conn_handle_events_server(connection_s2c *c) {
  payload_event pl_ev;
  package p;

  package_clean(&p);
  p.type = PACKAGE_EVENT;
  p.payload_size = sizeof(payload_event);
  p.payload = &pl_ev;

  while (conn_dequeue_event(&c->c, &pl_ev.ev)) {
	send_package(&c->c, &p);
  }
}

void
conn_handle_events_client(connection_c2s *conn) {
  DTODO_PRINTF("TODO: this");// TODO: this
}

void
conn_notify_join(connection_s2c *c, player *pl) {
  package p;

  if (!c->c.active)
	return;

  package_clean(&p);
  p.type = PACKAGE_NOTIFY_JOIN;

  payload_notify_join pl_nj;
  copy_player_id(&pl_nj.pid, &pl->id);

  p.payload_size = sizeof(payload_notify_join);
  p.payload = &pl_nj;

  send_package(&c->c, &p);
}

void
conn_notify_disconnect(connection_s2c *c, player *pl) {
  package p;

  if (!c->c.active)
	return;

  package_clean(&p);
  p.type = PACKAGE_NOTIFY_LEAVE;

  payload_notify_leave pl_nl;
  copy_player_id(&pl_nl.pid, &pl->id);

  p.payload_size = sizeof(payload_notify_leave);
  p.payload = &pl_nl;

  send_package(&c->c, &p);
}

void
conn_disable_conn(connection *c) {
  close(c->fd);
  c->active = 0;
  clear_action_queue(&c->aq);
  clear_event_queue(&c->eq);
}

int
conn_dequeue_action(connection *c, action *a) {
  return dequeue_action(&c->aq, a);
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
