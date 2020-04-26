
#pragma once

#include "action.h"
#include "atomic_queue.h"
#include "event.h"
#include "skat.h"
#include <pthread.h>
#include <stdint.h>

typedef int64_t seq_no;

struct server;
typedef struct server server;

typedef enum {
  REQ_RSP_INVALID = 0,
  REQ_RSP_ERROR,
  REQ_RSP_RESYNC,
  REQ_RSP_JOIN,
  REQ_RSP_CONFIRM_JOIN,
  REQ_RSP_NOTIFY_JOIN,
  REQ_RSP_CONN_RESUME,
  REQ_RSP_CONFIRM_RESUME,
  REQ_RSP_ACTION,
  REQ_RSP_EVENT,
  REQ_RSP_DISCONNECT,
  REQ_RSP_NOTIFY_LEAVE
} req_rsp_type;

typedef enum {
  CONN_ERROR_INVALID = 0,
  CONN_ERROR_INVALID_CONN_STATE,
  CONN_ERROR_PLAYER_ID_IN_USE,
  CONN_ERROR_NO_SUCH_PLAYER_ID,
  CONN_ERROR_INCONSISTENT_SEQ_NUM,
  CONN_ERROR_INVALID_PACKAGE_TYPE
} conn_error_type;

typedef struct {
  seq_no seq;
  union {
	player_id pid;
	action ac;
	event ev;
  };
} request;

typedef struct {
  seq_no seq;
  union {
	conn_error_type cet;
	skat_client_state scs;
  };
} response;

typedef struct {
  req_rsp_type type;
  request req;
  response rsp;
} package;

typedef struct {
  int fd;
  seq_no cseq;
  pthread_t handler;
} connection;

typedef struct {
  connection c;
  int active;
  action_queue aq;
  event_queue eq;
  player_id pid;
} connection_s2c;

typedef struct {
  connection c;
} connection_c2s;

connection_s2c *
establish_connection_server(server *, int, pthread_t);

int
conn_handle_incoming_packages_server(server *, connection_s2c *);
void
conn_handle_events_server(connection_s2c *);

void
conn_notify_join(connection_s2c *, player *);
void
conn_notify_disconnect(connection_s2c *, player *);

void
conn_disable_conn(connection_s2c *);

int
conn_dequeue_action(connection_s2c *, action *);
void
conn_enqueue_event(connection_s2c *, event *);
void
conn_enqueue_action(connection_s2c *, action *);
int
conn_dequeue_event(connection_s2c *, event *);
