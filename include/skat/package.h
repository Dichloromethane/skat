#ifndef PACKAGE_C_HDR
#define PACKAGE_C_HDR

#include "skat/byte_buf.h"
#include "skat/connection.h"
#include <stdint.h>
#include <stdlib.h>

#ifndef STRINGIFY
#define STRINGIFY_   #x
#define STRINGIFY(x) STRINGIFY_(x)
#endif

// clang-format off
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
  PACKAGE(JOIN),
  PACKAGE(CONFIRM_JOIN),
  PACKAGE(RESUME),
  PACKAGE(CONFIRM_RESUME),
  PACKAGE(RESYNC),
  PACKAGE(NOTIFY_JOIN),
  PACKAGE(NOTIFY_LEAVE),
  PACKAGE(ACTION),
  PACKAGE(EVENT),
  PACKAGE(REQUEST_RESYNC),
  PACKAGE(CONFIRM_RESYNC),
  PACKAGE(DISCONNECT)
PACKAGE_HDR_TABLE_END

#ifndef PACKAGE_HDR_TO_STRING
; // to make clang-format happy
// clang-format on

extern char *package_name_table[];

typedef struct {
  conn_error_type type;
} payload_error;

typedef struct {
  uint16_t network_protocol_version;
  char *name;
} payload_join;

typedef struct {
  int8_t gupid;
} payload_confirm_join;

typedef payload_join payload_resume;

typedef payload_confirm_join payload_confirm_resume;

typedef struct {
  skat_client_state scs;
  int8_t aps[4];// gupid -> ap
  char *player_names[4];
} payload_resync;

typedef struct payload_notify_join {
  int8_t gupid;
  int8_t ap;
  char *name;
} payload_notify_join;

typedef struct payload_notify_leave {
  int8_t gupid;
} payload_notify_leave;

typedef struct {
  action ac;
} payload_action;

typedef struct {
  event ev;
} payload_event;

typedef struct {
  package_type type;
  union {
	payload_error pl_er;
	payload_join pl_j;
	payload_resume pl_rm;
	payload_notify_join pl_nj;
	payload_notify_leave pl_nl;
	payload_confirm_join pl_cj;
	payload_confirm_resume pl_cr;
	payload_resync pl_rs;
	payload_event pl_ev;
	payload_action pl_a;
  };
} package;

void package_clean(package *);
void package_free(package *);

void package_read(package *p, byte_buf *bb);
void package_write(const package *p, byte_buf *bb);

#endif
#endif
