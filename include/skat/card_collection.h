#pragma once

#include "skat/card.h"
#include <stddef.h>
#include <stdint.h>

typedef uint32_t card_collection;

int card_collection_contains(const card_collection *, const card_id *, int *);
int card_collection_add_card(card_collection *, const card_id *);
int card_collection_add_card_array(card_collection *, const card_id *, size_t);
int card_collection_remove_card(card_collection *, const card_id *);
int card_collection_get_card_count(const card_collection *, uint8_t *);
int card_collection_get_card(const card_collection *, const uint8_t *,
							 card_id *);
int card_collection_get_score(const card_collection *, unsigned int *);
int card_collection_empty(card_collection *);
int card_collection_fill(card_collection *);
int card_collection_draw_random(const card_collection *, card_id *);