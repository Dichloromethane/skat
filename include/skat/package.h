#ifndef PACKAGE_C_HDR
#define PACKAGE_C_HDR

#define _GNU_SOURCE

#include "skat/connection.h"
#include <stdint.h>
#include <stdlib.h>

// clang-format off
#ifndef STRINGIFY
#define STRINGIFY_ #x
#define STRINGIFY(x) STRINGIFY_(x)
#endif

#ifdef PACKAGE_HDR_TO_STRING
#undef PACKAGE_HDR_TABLE_BEGIN
  #undef FIRST_PACKAGE
  #undef PACKAGE
  #undef PACKAGE_HDR_TABLE_END

  #define PACKAGE_HDR_TABLE_BEGIN char *package_name_table[] = {
  #define FIRST_PACKAGE(x) PACKAGE(x)
  #define PACKAGE(x) [PACKAGE_ ## x] = "PACKAGE_" #x
  #define PACKAGE_HDR_TABLE_END , (void *)0};
#else
#define PACKAGE_HDR_TABLE_BEGIN typedef enum {
#define FIRST_PACKAGE(x) PACKAGE_ ## x = 0
#define PACKAGE(x) PACKAGE_ ## x
#define PACKAGE_HDR_TABLE_END } package_type;
#endif

PACKAGE_HDR_TABLE_BEGIN
  FIRST_PACKAGE(INVALID),
  PACKAGE(ERROR),
  PACKAGE(RESYNC),
  PACKAGE(CONFIRM_RESYNC),
  PACKAGE(JOIN),
  PACKAGE(CONFIRM_JOIN),
  PACKAGE(NOTIFY_JOIN),
  PACKAGE(CONN_RESUME),
  PACKAGE(CONFIRM_RESUME),
  PACKAGE(ACTION),
  PACKAGE(EVENT),
  PACKAGE(DISCONNECT),
  PACKAGE(NOTIFY_LEAVE)
PACKAGE_HDR_TABLE_END

#ifndef PACKAGE_HDR_TO_STRING
; // to make clang-format happy
// clang-format on

extern char *package_name_table[];

typedef struct {
  conn_error_type type;
} payload_error;

typedef struct {
  player_name pname;
} payload_join;

typedef payload_join payload_resume;

typedef struct {
  int gupid;
  player_name pname;
} payload_notify_join;

typedef payload_notify_join payload_notify_leave;

typedef struct {
  int gupid;
} payload_confirm_join;

typedef payload_confirm_join payload_confirm_resume;

typedef struct {
  skat_client_state scs;
} payload_resync;

typedef struct {
  event ev;
} payload_event;

typedef struct {
  action ac;
} payload_action;

typedef struct {
  package_type type;
  size_t payload_size;
  union {
	void *v;
	payload_error *pl_er;
	payload_join *pl_j;
	payload_resume *pl_rm;
	payload_notify_join *pl_nj;
	payload_notify_leave *pl_nl;
	payload_confirm_join *pl_cj;
	payload_confirm_resume *pl_cr;
	payload_resync *pl_rs;
	payload_event *pl_ev;
	payload_action *pl_a;
  } payload;
} package;

void package_clean(package *);
void package_free(package *);

#endif
#endif
