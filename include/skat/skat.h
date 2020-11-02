#ifndef SKAT_HDR
#define SKAT_HDR

#include "skat/action.h"
#include "skat/card.h"
#include "skat/card_collection.h"
#include "skat/event.h"
#include "skat/game_rules.h"
#include "skat/player.h"
#include "skat/reizen.h"
#include "skat/stich.h"

#ifndef STRINGIFY
#define STRINGIFY_   #x
#define STRINGIFY(x) STRINGIFY_(x)
#endif

// clang-format off
#ifdef GAME_PHASE_HDR_TO_STRING
  #undef GAME_PHASE_HDR_TABLE_BEGIN
  #undef FIRST_GAME_PHASE
  #undef GAME_PHASE
  #undef GAME_PHASE_HDR_TABLE_END
  #define GAME_PHASE_HDR_TABLE_BEGIN char *game_phase_name_table[] = {
  #define FIRST_GAME_PHASE(x) GAME_PHASE(x)
  #define GAME_PHASE(x) [GAME_PHASE_ ## x] = "GAME_PHASE_" #x
  #define GAME_PHASE_HDR_TABLE_END , NULL};
#else
  #define GAME_PHASE_HDR_TABLE_BEGIN typedef enum {
  #define FIRST_GAME_PHASE(x) GAME_PHASE_ ## x = 0
  #define GAME_PHASE(x) GAME_PHASE_ ## x
  #define GAME_PHASE_HDR_TABLE_END } game_phase;
#endif


GAME_PHASE_HDR_TABLE_BEGIN
  FIRST_GAME_PHASE(INVALID),
  GAME_PHASE(SETUP),
  GAME_PHASE(BETWEEN_ROUNDS),
  GAME_PHASE(REIZEN),
  GAME_PHASE(SKAT_AUFNEHMEN),
  GAME_PHASE(PLAY_START),
  GAME_PHASE(PLAY_STICH_C1),
  GAME_PHASE(PLAY_STICH_C2),
  GAME_PHASE(PLAY_STICH_C3),
  GAME_PHASE(CLIENT_WAIT_REIZEN_DONE),
  GAME_PHASE(CLIENT_WAIT_STICH_DONE),
  GAME_PHASE(CLIENT_WAIT_ROUND_DONE),
  GAME_PHASE(END)
GAME_PHASE_HDR_TABLE_END

#ifndef GAME_PHASE_HDR_TO_STRING
; // to make clang-format happy
// clang-format on

extern char *game_phase_name_table[];

typedef struct server server;
typedef struct client client;

typedef struct shared_game_state {
  game_phase cgphase;

  reiz_state rs;
  game_rules gr;

  int active_players[3];// map active player -> gupid

  int score[4];// indexed by gupid

  stich curr_stich;
  stich last_stich;
  int stich_num;    // 0-9
  int alleinspieler;// indexed by active player
  int took_skat;
} shared_game_state;

typedef struct skat_client_state {
  shared_game_state sgs;
  card_collection my_hand;
  card_collection my_stiche;
  int my_gupid;              // gupid
  int my_active_player_index;// active_player
  int my_partner;            // active_player
  int ist_alleinspieler;
  card_id skat[2];
} skat_client_state;

typedef struct skat_server_state {
  shared_game_state sgs;
  card_collection player_hands[3];// indexed by active player
  card_id skat[2];

  card_collection initial_alleinspieler_hand;

  // indexed by active player, has to be initialized after reizen
  card_collection *stiche[3];

  card_collection stiche_buf[3];// indexed via *stiche (3 weil ramschen)
} skat_server_state;

void skat_state_notify_disconnect(skat_server_state *, player *, server *);
void skat_state_notify_join(skat_server_state *, player *, server *);

struct payload_notify_join;
typedef struct payload_notify_join payload_notify_join;
void client_skat_state_notify_join(skat_client_state *, payload_notify_join *);

typedef struct payload_notify_leave payload_notify_leave;
void client_skat_state_notify_leave(skat_client_state *,
									payload_notify_leave *);

int skat_server_state_apply(skat_server_state *ss, action *a, player *pl,
							server *s);
void skat_server_state_tick(skat_server_state *ss, server *s);

int skat_client_state_apply(skat_client_state *cs, event *e, client *s);
void skat_client_state_tick(skat_client_state *cs, client *c);

void skat_resync_player(skat_server_state *, skat_client_state *, player *);

void server_skat_state_init(skat_server_state *ss);
void client_skat_state_init(skat_client_state *cs);

#endif
#endif
