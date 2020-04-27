#pragma once

typedef enum {
  COLOR_INVALID = 0,
  COLOR_KARO,
  COLOR_HERZ,
  COLOR_PIK,
  COLOR_KREUZ
} card_color;

typedef enum {
  CARD_TYPE_INVALID = 0,
  CARD_TYPE_7,
  CARD_TYPE_8,
  CARD_TYPE_9,
  CARD_TYPE_B,
  CARD_TYPE_D,
  CARD_TYPE_K,
  CARD_TYPE_10,
  CARD_TYPE_A
} card_type;

typedef struct {
  card_color cc;
  card_type ct;
} card;

typedef unsigned char card_id;
typedef unsigned int card_collection;

typedef struct stich stich;
typedef struct game_rules game_rules;

int card_get_id(const card *, card_id *);
int card_get(const card_id *, card *);
int card_get_name(const card_id *, char *);
int card_get_score(const card_id *, int *);

int card_collection_contains(const card_collection *, const card_id *, int *);
int card_collection_add_card(card_collection *, const card_id *);
int card_collection_add_card_array(card_collection *, const card_id *, int);
int card_collection_remove_card(card_collection *, const card_id *);
int card_collection_get_card_count(const card_collection *, int *);
int card_collection_get_card(const card_collection *, unsigned int, card_id *);
int card_collection_get_score(const card_collection *, int *);
int card_collection_empty(card_collection *);
int card_collection_fill(card_collection *);
int card_collection_draw_random(const card_collection *, card_id *);

int stich_get_winner(const game_rules *, const stich *, int *);
int stich_card_legal(const game_rules *, const card_id *, const int *,
					 const card_id *, const card_collection *, int *);
