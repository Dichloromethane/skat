
#ifndef SKAT_HDR
#define SKAT_HDR

#define _GNU_SOURCE

#include "skat/action.h"
#include "skat/card.h"
#include "skat/card_collection.h"
#include "skat/event.h"
#include "skat/game_rules.h"
#include "skat/player.h"
#include "skat/stich.h"

#ifndef STRINGIFY
#define STRINGIFY_ #x
#define STRINGIFY(x) STRINGIFY_(x)
#endif

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
  GAME_PHASE(REIZEN_BEGIN),
  GAME_PHASE(SKAT_AUFNEHMEN),
  GAME_PHASE(PLAY_START),
  GAME_PHASE(PLAY_STICH_C1),
  GAME_PHASE(PLAY_STICH_C2),
  GAME_PHASE(PLAY_STICH_C3),
  GAME_PHASE(END)
GAME_PHASE_HDR_TABLE_END

#ifndef GAME_PHASE_HDR_TO_STRING

extern char *game_phase_name_table[];

typedef struct server server;

typedef struct {
  int reizwert;
  unsigned hand : 1;
  unsigned schneider : 1;
  unsigned schwarz : 1;
  unsigned ouvert : 1;
  unsigned contra : 1;
  unsigned re : 1;
} reiz_resultat;

typedef struct {
  game_phase cgphase;
  game_rules gr;
  reiz_resultat rr;

  int active_players[3]; // map active player -> gupid
  int spectator; // ib gupid

  int score[4]; // indexed by gupid

  stich curr_stich;
  stich last_stich;
  int stich_num; // 0-9
  int alleinspieler; // indexed by active player
} shared_game_state;

typedef struct {
  shared_game_state sgs;
  card_collection my_hand;
  int my_index;
  union {
    card_id skat[2];
  }
} skat_client_state;

typedef struct {
  shared_game_state sgs;
  card_collection player_hands[3]; // indexed by active player
  card_id skat[2]; 
  card_collection *stiche[3]; // indexed by active player
  card_collection stiche_buf[3]; // indexed via *stiche (3 weil ramschen)
  int spielwert;
} skat_state;

int game_setup_server(skat_state *ss);
int game_start_server(skat_state *ss);

typedef void (*send_event_f)(event *, void (*)(event *, player *));

void skat_state_notify_disconnect(skat_state *, player *, server *);
void skat_state_notify_join(skat_state *, player *, server *);

int skat_state_apply(skat_state *, action *, player *, server *);
void skat_state_tick(skat_state *, server *);

void skat_resync_player(skat_state *, skat_client_state *, player *);

void skat_state_init(skat_state *);

#endif
#endif
