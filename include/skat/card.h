#pragma once

#include <stddef.h>
#include <stdint.h>

typedef enum card_color {
  COLOR_INVALID = 0,
  COLOR_KARO,
  COLOR_HERZ,
  COLOR_PIK,
  COLOR_KREUZ
} card_color;

typedef enum card_type {
  CARD_TYPE_INVALID = 0,// enum index, card order normal, card order null
  CARD_TYPE_7,          // 1, 0, 0
  CARD_TYPE_8,          // 2, 1, 1
  CARD_TYPE_9,          // 3, 2, 2
  CARD_TYPE_D,          // 4, 3, 5
  CARD_TYPE_K,          // 5, 4, 6
  CARD_TYPE_10,         // 6, 5, 3
  CARD_TYPE_A,          // 7, 6, 7
  CARD_TYPE_B           // 8, 7, 4
} card_type;

typedef struct card {
  card_type ct; // type
  card_color cc;// color
} card;

typedef enum card_sort_mode {
  CARD_SORT_MODE_ID,
  CARD_SORT_MODE_PREGAME_HAND,
  CARD_SORT_MODE_INGAME_HAND,
  CARD_SORT_MODE_STICHE
} card_sort_mode;

typedef struct game_rules game_rules;
typedef struct {
  const game_rules *const gr;
  const card_sort_mode *const mode;
} card_compare_args;

typedef uint8_t card_id;
#define CARD_ID_MAX (72)

int card_get_id(const card *, card_id *);
int card_get(const card_id *, card *);
int card_get_name(const card_id *, char *);
int card_get_score(const card_id *, uint8_t *);

int card_compare(const card_id *, const card_id *, const card_compare_args *);
