#pragma once

#include "skat/card.h"
#include "skat/card_collection.h"
#include "skat/game_rules.h"

typedef struct stich {
  card_id cs[3];// indexedby vorhand + active player
  int played_cards;
  int vorhand;// indexed active player
  int winner; // indexed active player
} stich;

int stich_get_winner(const game_rules *gr, const stich *stich, int *result);
int stich_card_legal(const game_rules *gr, const stich *stich,
					 const card_id *new_card, const card_collection *hand,
					 int *result);
