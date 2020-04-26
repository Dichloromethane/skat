
#pragma once

#include "action.h"
#include "card.h"
#include "event.h"
#include "game_rules.h"
#include "player.h"

struct server;
typedef struct server server;

typedef enum {
  GAME_PHASE_INVALID = 0,
  GAME_PHASE_SETUP,
  GAME_PHASE_BETWEEN_ROUNDS,
  GAME_PHASE_REIZEN_BEGIN,
  GAME_PHASE_PLAY_START,
  GAME_PHASE_PLAY_STICH_C1,
  GAME_PHASE_PLAY_STICH_C2,
  GAME_PHASE_PLAY_STICH_C3,
  GAME_PHASE_END
} game_phase;

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
  player_id active_players[3];
  int num_players;
  int last_active_player_index;
  union {
	struct {
	  card_id played_cards[3];
	  int stich_num;
	  int vorhand;
	  card_collection last_stich;
	  int alleinspieler;
	};
  };
} shared_game_state;

typedef struct {
  shared_game_state sgs;
  card_collection my_hand;
  card_id sc[2];
} skat_client_state;

typedef struct {
  shared_game_state sgs;
  card_collection player_hands[3];
  card_id skat[2];
  card_collection stiche_alleinspieler;
  card_collection stiche_other;
  int last_active_player_index;
  int spielwert;
} skat_state;

int
game_setup_server(skat_state *ss);
int
game_start_server(skat_state *ss);

int
rand_int(int min, int max);

typedef void (*send_event_f)(event *, void (*)(event *, player *));

void
skat_state_notify_disconnect(skat_state *, player *, server *);
void
skat_state_notify_join(skat_state *, player *, server *);

int
skat_state_apply(skat_state *, action *, player *, server *);
void
skat_state_tick(skat_state *, server *);
