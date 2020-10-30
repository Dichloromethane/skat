#pragma once

#include "skat/card.h"

typedef enum game_type {
  GAME_TYPE_INVALID = 0,
  GAME_TYPE_COLOR,
  GAME_TYPE_GRAND,
  GAME_TYPE_NULL,
  GAME_TYPE_RAMSCH
} game_type;

typedef struct game_rules {
  game_type type;
  card_color trumpf;
  unsigned hand : 1;
  unsigned schneider_angesagt : 1;
  unsigned schwarz_angesagt : 1;
  unsigned ouvert : 1;
} game_rules;
