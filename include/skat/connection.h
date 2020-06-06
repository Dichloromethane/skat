
#ifndef CONNECTION_C_HDR
#define CONNECTION_C_HDR

#define _GNU_SOURCE

#include "skat/action.h"
#include "skat/atomic_queue.h"
#include "skat/event.h"
#include "skat/skat.h"
#include <pthread.h>
#include <stdint.h>

#ifndef STRINGIFY
#define STRINGIFY_ #x
#define STRINGIFY(x) STRINGIFY_(x)
#endif

#ifdef CONNECTION_HDR_TO_STRING
  #undef REQ_RSP_HDR_TABLE_BEGIN
  #undef FIRST_REQ_RSP
  #undef REQ_RSP
  #undef REQ_RSP_HDR_TABLE_END

  #undef CONN_ERROR_HDR_TABLE_BEGIN
  #undef FIRST_CONN_ERROR
  #undef CONN_ERROR
  #undef CONN_ERROR_HDR_TABLE_END

  #define REQ_RSP_HDR_TABLE_BEGIN char *req_rsp_name_table[] = {
  #define FIRST_REQ_RSP(x) REQ_RSP(x)
  #define REQ_RSP(x) [REQ_RSP_ ## x] = "REQ_RSP_" #x
  #define REQ_RSP_HDR_TABLE_END , (void *)0};

  #define CONN_ERROR_HDR_TABLE_BEGIN char *conn_error_name_table[] = {
  #define FIRST_CONN_ERROR(x) CONN_ERROR(x)
  #define CONN_ERROR(x) [CONN_ERROR_ ## x] = "CONN_ERROR_" #x
  #define CONN_ERROR_HDR_TABLE_END , (void *)0};
#else
  #define REQ_RSP_HDR_TABLE_BEGIN typedef enum {
  #define FIRST_REQ_RSP(x) REQ_RSP_ ## x = 0
  #define REQ_RSP(x) REQ_RSP_ ## x
  #define REQ_RSP_HDR_TABLE_END } req_rsp_type;

  #define CONN_ERROR_HDR_TABLE_BEGIN typedef enum {
  #define FIRST_CONN_ERROR(x) CONN_ERROR_ ## x = 0
  #define CONN_ERROR(x) CONN_ERROR_ ## x
  #define CONN_ERROR_HDR_TABLE_END } conn_error_type;
#endif

REQ_RSP_HDR_TABLE_BEGIN
  FIRST_REQ_RSP(INVALID),
  REQ_RSP(ERROR),
  REQ_RSP(RESYNC),
  REQ_RSP(JOIN),
  REQ_RSP(CONFIRM_JOIN),
  REQ_RSP(NOTIFY_JOIN),
  REQ_RSP(CONN_RESUME),
  REQ_RSP(CONFIRM_RESUME),
  REQ_RSP(ACTION),
  REQ_RSP(EVENT),
  REQ_RSP(DISCONNECT),
  REQ_RSP(NOTIFY_LEAVE)
REQ_RSP_HDR_TABLE_END

CONN_ERROR_HDR_TABLE_BEGIN
  CONN_ERROR(INVALID),
  CONN_ERROR(INVALID_CONN_STATE),
  CONN_ERROR(PLAYER_ID_IN_USE),
  CONN_ERROR(NO_SUCH_PLAYER_ID),
  CONN_ERROR(INCONSISTENT_SEQ_NUM),
  CONN_ERROR(INVALID_PACKAGE_TYPE),
  CONN_ERROR(TOO_MANY_PLAYERS)
CONN_ERROR_HDR_TABLE_END

#ifndef CONNECTION_HDR_TO_STRING

extern char *conn_error_name_table[];
extern char *req_rsp_name_table[];

typedef int64_t seq_no;

typedef struct server server;
typedef struct client client;

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

connection_s2c *establish_connection_server(server *, int, pthread_t);
connection_c2s *establish_connection_client(client *, int, pthread_t, int);

int conn_handle_incoming_packages_server(server *, connection_s2c *);
void conn_handle_events_server(connection_s2c *);

int conn_handle_incoming_packages_client(client *, connection_c2s *);
void conn_handle_events_client(connection_c2s *);

void conn_notify_join(connection_s2c *, player *);
void conn_notify_disconnect(connection_s2c *, player *);

void conn_disable_conn(connection_s2c *);

int conn_dequeue_action(connection_s2c *, action *);
void conn_enqueue_event(connection_s2c *, event *);
void conn_enqueue_action(connection_s2c *, action *);
int conn_dequeue_event(connection_s2c *, event *);

#endif
#endif
