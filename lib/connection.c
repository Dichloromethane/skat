#include "connection.h"
#include "server.h"
#include <pthread.h>
#include <stddef.h>
#include <string.h>
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

static void
send_package(connection *c, package *p) {
  send(c->fd, p, sizeof(package), 0);
}

static int
retrieve_package(connection *c, package *p) {
  size_t res;
  res = read(c->fd, p, sizeof(package));
  return res == sizeof(package);
}

static void
conn_error(connection *c, conn_error_type cet) {
  package p;
  p.type = REQ_RSP_ERROR;
  p.rsp.cet = cet;
  p.rsp.seq = ++c->cseq;
  send_package(c, &p);
}

static void
init_conn_s2c(connection_s2c *c) {
  init_action_queue(&c->aq);
  init_event_queue(&c->aq);
  c->active = 0;
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
	CH_ASSERT_NULL(!server_has_player_id(s, p.req.pid), &c,
				   CONN_ERROR_PLAYER_ID_IN_USE);

	memcpy(&pl.id, &p.req.pid, sizeof(player_id));

	s2c = server_get_free_connection(s);
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
conn_notify_join(connection_s2c *c, player *pl) {
  package p;
  if (!c->active)
	return;
  p.type = REQ_RSP_NOTIFY_JOIN;
  p.req.seq = ++c->c.cseq;
  memcpy(&p.req.pid, &pl->id, sizeof(player_id));
  send_package(&c->c, &p);
}

void
conn_notify_disconnect(connection_s2c *c, player *pl) {
  package p;
  if (!c->active)
	return;
  p.type = REQ_RSP_NOTIFY_LEAVE;
  p.req.seq = ++c->c.cseq;
  memcpy(&p.req.pid, &pl->id, sizeof(player_id));
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
