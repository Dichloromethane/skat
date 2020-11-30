#pragma once

#include "skat/card.h"
#include "skat/card_collection.h"

typedef enum card_color_mode {
  CARD_COLOR_MODE_NONE,
  CARD_COLOR_MODE_ONLY_CARD_COLOR,
  CARD_COLOR_MODE_PLAYABLE
} card_color_mode;

typedef struct shared_game_state shared_game_state;
void print_card_array(const shared_game_state *sgs, const card_collection *cc,
					  const card_id *arr, size_t length,
					  card_color_mode color_mode);
void print_card_collection(const shared_game_state *sgs,
						   const card_collection *cc, card_sort_mode sort_mode,
						   card_color_mode color_mode);
