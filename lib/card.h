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

typedef unsigned int card_collection;

typedef char card_id;

int card_get_id(card *, card_id *);
int card_get(card_id *, card *);
int card_get_name(card_id *, char *);
int card_get_score(card_id *, int *);

int card_collection_contains(card_collection *, card_id *, int *);
int card_collection_add_card(card_collection *, card_id *);
int card_collection_add_card_array(card_collection *, card_id *, int);
int card_collection_remove_card(card_collection *, card_id *);
int card_collection_get_card_count(card_collection *, int *);
int card_collection_get_card(card_collection *, int, card_id *);
int card_collection_get_score(card_collection *, int *);
int card_collection_empty(card_collection *);
int card_collection_fill(card_collection *);
int card_collection_draw_random(card_collection *, card_id *);

typedef struct game_rules game_rules;

int stich_card_legal(card_id *, card_id, card_collection *, int, game_rules *);
int stich_get_winner(game_rules *, card_id *, int *);
