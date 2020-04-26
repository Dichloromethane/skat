
#pragma once

#include "player.h"
#include "action.h"
#include "card.h"

typedef enum {
  EVENT_INVALID = 0,
  EVENT_ILLEGAL_ACTION,
  EVENT_SUSPEND_GAME,
  EVENT_START_GAME,
  EVENT_START_ROUND,
  EVENT_PLAYER_READY,
  EVENT_DISTRIBUTE_CARDS,
  EVENT_PLAY_CARD
} event_type;

typedef struct {
  event_type type;
  action_id answer_to;
  player_id player;
  union {
	player_id ready_player;
	player_id current_active_players[3];
	card_collection hand;
	card_id card;
  };
} event;
