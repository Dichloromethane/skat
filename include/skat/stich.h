#pragma once

#include "skat/byte_buf.h"
#include "skat/card.h"
#include "skat/card_collection.h"
#include "skat/game_rules.h"

typedef struct stich {
  card_id cs[3];// indexed by vorhand + active player
  uint8_t played_cards;
  int8_t vorhand;// indexed by active player
  int8_t winner; // indexed by active player
} stich;

int stich_get_winner(const game_rules *gr, const stich *stich, int *result);
int stich_card_legal(const game_rules *gr, const stich *stich,
					 const card_id *new_card, const card_collection *hand,
					 int *result);

void read_stich(struct stich *this, byte_buf *bb);
void write_stich(const struct stich *this, byte_buf *bb);
