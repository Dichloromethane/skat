#ifndef EVENT_HDR
#define EVENT_HDR

#include "skat/action.h"
#include "skat/card.h"
#include "skat/card_collection.h"
#include "skat/game_rules.h"
#include "skat/player.h"
#include "skat/reizen.h"
#include <stdlib.h>

#ifndef STRINGIFY
#define STRINGIFY_   #x
#define STRINGIFY(x) STRINGIFY_(x)
#endif

// clang-format off
#ifdef EVENT_HDR_TO_STRING
  #undef EVENT_HDR_TABLE_BEGIN
  #undef FIRST_EVENT
  #undef EVENT
  #undef EVENT_HDR_TABLE_END
  #define EVENT_HDR_TABLE_BEGIN char *event_name_table[] = {
  #define FIRST_EVENT(x) EVENT(x)
  #define EVENT(x) [EVENT_ ## x] = "EVENT_" #x
  #define EVENT_HDR_TABLE_END , (void *)0};
#else
  #define EVENT_HDR_TABLE_BEGIN typedef enum {
  #define FIRST_EVENT(x) EVENT_ ## x = 0
  #define EVENT(x) EVENT_ ## x
  #define EVENT_HDR_TABLE_END } event_type;
#endif

EVENT_HDR_TABLE_BEGIN
  FIRST_EVENT(INVALID),
  EVENT(ILLEGAL_ACTION),
  EVENT(START_GAME),
  EVENT(START_ROUND),
  EVENT(DISTRIBUTE_CARDS),
  EVENT(REIZEN_NUMBER),
  EVENT(REIZEN_CONFIRM),
  EVENT(REIZEN_PASSE),
  EVENT(REIZEN_DONE),
  EVENT(SKAT_TAKE),
  EVENT(SKAT_LEAVE),
  EVENT(SKAT_PRESS),
  EVENT(PLAY_CARD),
  EVENT(STICH_DONE),
  EVENT(ANNOUNCE_SCORES),
  EVENT(ROUND_DONE),
  EVENT(GAME_CALLED)
EVENT_HDR_TABLE_END

#ifndef EVENT_HDR_TO_STRING
; // to make clang-format happy
// clang-format on

typedef struct {
  event_type type;
  action_id answer_to;
  int acting_player;// gupid
  union {
	int current_active_players[3];// indexed by ap, contains gupid
	card_collection hand;
	uint16_t reizwert;
	struct {
	  int alleinspieler;// ap
	  uint16_t reizwert_final;
	};
	card_id skat[2];
	card_id skat_press_cards[2];
	card_id card;
	int stich_winner;  // gupid
	int score_total[4];// indexed by gupid
	game_rules gr;
	round_result rr;
  };
} event;

extern char *event_name_table[];

#endif
#endif
