#pragma once

#include "skat/byte_buf.h"
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

void read_game_rules(game_rules *this, byte_buf *bb);
void write_game_rules(const game_rules *this, byte_buf *bb);
