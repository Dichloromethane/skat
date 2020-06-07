#pragma once

#include "skat/card.h"
#include "skat/card_collection.h"
#include "skat/game_rules.h"

typedef struct stich {
  card_id cs[3]; // indexed index by vorhand + active player
  int vorhand; // indexed active player
  int winner; // indexed active player
} stich;

int stich_get_winner(const game_rules *, const stich *, int *);
int stich_card_legal(const game_rules *, const card_id *, int,
					 const card_id *, const card_collection *, int *);
