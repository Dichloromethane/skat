#pragma once

#define _GNU_SOURCE

#include "skat/card.h"

typedef enum {
  GAME_TYPE_INVALID = 0,
  GAME_TYPE_COLOR,
  GAME_TYPE_GRAND,
  GAME_TYPE_NULL,
  GAME_TYPE_RAMSCH
} game_type;

typedef struct game_rules {
  game_type type;
  card_color trumpf;
} game_rules;
