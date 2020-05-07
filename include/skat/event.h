
#ifndef EVENT_HDR
#define EVENT_HDR

#include <stdlib.h>
#include "skat/action.h"
#include "skat/card.h"
#include "skat/card_collection.h"
#include "skat/player.h"


#ifndef STRINGIFY
#define STRINGIFY_ #x
#define STRINGIFY(x) STRINGIFY_(x)
#endif

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
  EVENT(SUSPEND_GAME),
  EVENT(START_GAME),
  EVENT(START_ROUND),
  EVENT(PLAYER_READY),
  EVENT(DISTRIBUTE_CARDS),
  EVENT(PLAY_CARD),
  EVENT(STICH_DONE),
  EVENT(ROUND_DONE)
EVENT_HDR_TABLE_END

#ifndef EVENT_HDR_TO_STRING

typedef struct {
  event_type type;
  action_id answer_to;
  player_id player;
  union {
	player_id ready_player;
	player_id current_active_players[3];
	card_collection hand;
	card_id card;
	player_id stich_winner;
	int score_round[3];
  };
} event;

extern char *event_name_table[];

#endif
#endif
