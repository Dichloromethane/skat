#pragma once

#include "card.h"
#include <stdint.h>

typedef int64_t action_id;

typedef enum {
  ACTION_INVALID = 0,
  ACTION_READY,
  ACTION_RULE_CHANGE,
  ACTION_PLAY_CARD
} action_type;

typedef struct {
  action_type type;
  action_id id;
  union {
	card_id card;
  };
} action;
