#ifndef CONNECTION_C_HDR
#define CONNECTION_C_HDR

#include "skat/action.h"
#include "skat/event.h"
#include "skat/skat.h"
#include <pthread.h>
#include <stdint.h>

#ifndef STRINGIFY
#define STRINGIFY_   #x
#define STRINGIFY(x) STRINGIFY_(x)
#endif

// clang-format off
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
  #define CONN_ERROR_HDR_TABLE_BEGIN typedef enum {
  #define FIRST_CONN_ERROR(x) CONN_ERROR_ ## x = 0
  #define CONN_ERROR(x) CONN_ERROR_ ## x
  #define CONN_ERROR_HDR_TABLE_END } conn_error_type;
#endif

CONN_ERROR_HDR_TABLE_BEGIN
  CONN_ERROR(INVALID),
  CONN_ERROR(INVALID_CONN_STATE),
  CONN_ERROR(PLAYER_NAME_IN_USE),
  CONN_ERROR(NO_SUCH_PLAYER_NAME),
  CONN_ERROR(INCONSISTENT_SEQ_NUM),
  CONN_ERROR(INVALID_PACKAGE_TYPE),
  CONN_ERROR(TOO_MANY_PLAYERS),
  CONN_ERROR(DISCONNECTED)
CONN_ERROR_HDR_TABLE_END

#ifndef CONNECTION_HDR_TO_STRING
; // to make clang-format happy
// clang-format on

extern char *conn_error_name_table[];

#define TYPE action
#include "atomic_queue_header.def"
#undef TYPE

#define TYPE event
#include "atomic_queue_header.def"
#undef TYPE

typedef struct server server;
typedef struct client client;

typedef struct {
  int fd;
  pthread_t handler;
  int active;
  action_queue aq;
  event_queue eq;
} connection;

typedef struct {
  connection c;
  int gupid;
} connection_s2c;

typedef struct {
  connection c;
} connection_c2s;

connection_s2c *establish_connection_server(server *, int, pthread_t);
connection_c2s *establish_connection_client(client *, int, pthread_t, int);

int conn_handle_incoming_packages_server(server *, connection_s2c *);
_Noreturn void conn_handle_events_server(connection_s2c *);

int conn_handle_incoming_packages_client(client *, connection_c2s *);
_Noreturn void conn_handle_actions_client(connection_c2s *);

void conn_notify_join(connection_s2c *, player *);
void conn_notify_disconnect(connection_s2c *, player *);

void conn_disable_conn(connection *);

int conn_dequeue_action(connection *, action *);
void conn_dequeue_action_blocking(connection *, action *);
void conn_enqueue_event(connection *, event *);
void conn_enqueue_action(connection *, action *);
int conn_dequeue_event(connection *, event *);
void conn_dequeue_event_blocking(connection *, event *);

#endif
#endif
