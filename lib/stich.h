#pragma once

#include "card.h"
#include "card_collection.h"
#include "game_rules.h"

typedef struct stich {
  card_id cs[3];
  int vorhand;
  int winner;
} stich;

int stich_get_winner(const game_rules *, const stich *, int *);
int stich_card_legal(const game_rules *, const card_id *, const int *,
					 const card_id *, const card_collection *, int *);