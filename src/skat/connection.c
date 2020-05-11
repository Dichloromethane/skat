#include "skat/connection.h"
#include "skat/client.h"
#include "skat/package_queue.h"
#include "skat/server.h"
#include "skat/util.h"
#include <stddef.h>
#include <sys/socket.h>
#include <unistd.h>

#define SEQ_NUM_START (1)

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
  DEBUG_PRINTF("Sending package of type %s", req_rsp_name_table[p->type]);
  send(c->fd, p, sizeof(package), 0);
}

static int
retrieve_package(connection *c, package *p) {
  size_t res;
  res = read(c->fd, p, sizeof(package));
  DEBUG_PRINTF("Retrieved package of type %s with size %ld",
			   req_rsp_name_table[p->type], res);
  return res == sizeof(package);
}

static void
conn_error(connection *c, conn_error_type cet) {
  package p;
  p.type = REQ_RSP_ERROR;
  p.rsp.cet = cet;
  p.rsp.seq = ++c->cseq;
  DEBUG_PRINTF("Connection error: %s", conn_error_name_table[cet]);
  send_package(c, &p);
}

static void
init_conn_s2c(connection_s2c *c) {
  init_action_queue(&c->aq);
  init_event_queue(&c->aq);
  c->active = 0;
}

static void
init_conn_c2s(connection_c2s *s) {
  // init_action_queue(&s->aq);
  // init_event_queue(&s->aq);
  // s->active = 0;
}

connection_s2c *
establish_connection_server(server *s, int fd, pthread_t handler) {
  package p, re;
  connection c;
  connection_s2c *s2c;
  player pl;

  c.fd = fd;
  c.cseq = SEQ_NUM_START;
  c.handler = handler;

  retrieve_package(&c, &p);
  CH_ASSERT_NULL(p.req.seq == SEQ_NUM_START, &c,
				 CONN_ERROR_INCONSISTENT_SEQ_NUM);

  if (p.type == REQ_RSP_JOIN) {
	server_acquire_state_lock(s);

	copy_player_id(&pl.id, &p.req.pid);

	CH_ASSERT_NULL(!server_has_player_id(s, &pl.id), &c,
				   CONN_ERROR_PLAYER_ID_IN_USE);

	CH_ASSERT_NULL(s2c = server_get_free_connection(s), &c,
				   CONN_ERROR_TOO_MANY_PLAYERS);

	init_conn_s2c(s2c);
	s2c->c = c;
	s2c->active = 1;
	s2c->pid = pl.id;

	server_add_player(s, &pl);

	server_notify_join(s, &pl);

	server_release_state_lock(s);

	re.type = REQ_RSP_CONFIRM_JOIN;
	re.rsp.seq = p.req.seq;
	send_package(&s2c->c, &re);
	return s2c;
  }
  if (p.type == REQ_RSP_CONN_RESUME) {
	server_acquire_state_lock(s);
	CH_ASSERT_NULL(s2c = server_get_connection_by_pid(s, p.req.pid), &c,
				   CONN_ERROR_NO_SUCH_PLAYER_ID);
	CH_ASSERT_NULL(!s2c->active, &c, CONN_ERROR_PLAYER_ID_IN_USE);

	init_conn_s2c(s2c);
	s2c->c = c;
	s2c->active = 1;

	server_notify_join(s, &pl);

	server_release_state_lock(s);

	re.type = REQ_RSP_CONFIRM_RESUME;
	re.rsp.seq = p.req.seq;
	send_package(&s2c->c, &re);
	return s2c;
  }
  CH_ASSERT_NULL(0, &c, CONN_ERROR_INVALID_CONN_STATE);
}

static int
conn_handle_incoming_package_client_single(client *c, connection_c2s *conn,
										   package *p) {
  switch (p->type) {
	// TODO: this
	default:
	  CH_ASSERT_0(0, &conn->c, CONN_ERROR_INVALID_PACKAGE_TYPE);
  }
  return 1;
}

int
conn_handle_incoming_packages_client(client *c, connection_c2s *conn) {
  package p;
  int still_connected;
  still_connected = retrieve_package(&conn->c, &p);
  if (!still_connected) {
	client_acquire_state_lock(c);
	client_disconnect_connection(c, conn);
	client_release_state_lock(c);
	return 0;
  }
  return conn_handle_incoming_package_client_single(c, conn, &p);
}

connection_c2s *
establish_connection_client(client *c, int socket_fd, pthread_t handler,
							int resume) {
  DEBUG_PRINTF("%s connection to server",
			   resume ? "Resuming" : "Establishing new");
  connection conn;
  package_queue pq;

  conn.fd = socket_fd;
  conn.cseq = SEQ_NUM_START;
  conn.handler = handler;

  connection_c2s *conn_c2s = malloc(sizeof(connection_c2s));

  init_conn_c2s(conn_c2s);
  conn_c2s->c = conn;

  package p;
  player_id pid;
  init_player_id(&pid, c->name);

  p.req.seq = SEQ_NUM_START;
  p.req.pid = pid;
  p.type = resume ? REQ_RSP_CONN_RESUME : REQ_RSP_JOIN;

  send_package(&conn_c2s->c, &p);

  p.req.seq = ++conn.cseq;
  p.type = REQ_RSP_RESYNC;

  send_package(&conn_c2s->c, &p);

  package_queue_init(&pq);

  retrieve_package(&conn_c2s->c, &p);
  while (p.type != REQ_RSP_RESYNC) {
	package_queue_enq(&pq, &p);
	retrieve_package(&conn_c2s->c, &p);
  }

  client_handle_resync(&p);

  while (package_queue_deq(&pq, &p)) {
	conn_handle_incoming_package_client_single(c, conn_c2s, &p);
  }

  return conn_c2s;
}

static void
conn_resync_player(server *s, connection_s2c *c, package *req_p) {
  package p;
  p.type = REQ_RSP_RESYNC;
  p.rsp.seq = req_p->req.seq;

  player *pl = server_get_player_by_pid(s, c->pid);
  server_resync_player(s, pl, &p.rsp.scs);

  send_package(&c->c, &p);
}

int
conn_handle_incoming_packages_server(server *s, connection_s2c *c) {
  package p;
  int still_connected;
  still_connected = retrieve_package(&c->c, &p);
  if (!still_connected) {
	server_acquire_state_lock(s);
	server_disconnect_connection(s, c);
	server_release_state_lock(s);
	return 0;
  }
  switch (p.type) {
	case REQ_RSP_ACTION:
	  conn_enqueue_action(c, &p.req.ac);
	  break;
	case REQ_RSP_RESYNC:
	  server_acquire_state_lock(s);
	  conn_resync_player(s, c, &p);
	  server_release_state_lock(s);
	  break;
	case REQ_RSP_ERROR:
	  DERROR_PRINTF("Received error from client, killing him");
	  __attribute__((fallthrough));
	case REQ_RSP_DISCONNECT:
	  server_acquire_state_lock(s);
	  server_disconnect_connection(s, c);
	  server_release_state_lock(s);
	  return 0;
	default:
	  CH_ASSERT_0(0, &c->c, CONN_ERROR_INVALID_PACKAGE_TYPE);
  }
  return 1;
}

void
conn_handle_events_server(connection_s2c *c) {
  package p;
  while (conn_dequeue_event(c, &p.req.ev)) {
	p.type = REQ_RSP_EVENT;
	p.req.seq = ++c->c.cseq;
	send_package(&c->c, &p);
  }
}


void
conn_handle_events_client(connection_c2s *conn) {
  // TODO: this
}

void
conn_notify_join(connection_s2c *c, player *pl) {
  package p;
  if (!c->active)
	return;
  p.type = REQ_RSP_NOTIFY_JOIN;
  p.req.seq = ++c->c.cseq;
  copy_player_id(&p.req.pid, &pl->id);
  send_package(&c->c, &p);
}

void
conn_notify_disconnect(connection_s2c *c, player *pl) {
  package p;
  if (!c->active)
	return;
  p.type = REQ_RSP_NOTIFY_LEAVE;
  p.req.seq = ++c->c.cseq;
  copy_player_id(&p.req.pid, &pl->id);
  send_package(&c->c, &p);
}

void
conn_disable_conn(connection_s2c *c) {
  close(c->c.fd);
  c->active = 0;
  clear_action_queue(&c->aq);
  clear_event_queue(&c->eq);
}

int
conn_dequeue_action(connection_s2c *c, action *a) {
  return dequeue_action(&c->aq, a);
}

void
conn_enqueue_event(connection_s2c *c, event *e) {
  enqueue_event(&c->eq, e);
}

void
conn_enqueue_action(connection_s2c *c, action *a) {
  enqueue_action(&c->aq, a);
}

int
conn_dequeue_event(connection_s2c *c, event *e) {
  return dequeue_event(&c->eq, e);
}
