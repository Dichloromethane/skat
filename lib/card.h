#pragma once

#include <stdint.h>

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

typedef uint8_t card_id;

int card_get_id(const card *, card_id *);
int card_get(const card_id *, card *);
int card_get_name(const card_id *, char *);
int card_get_score(const card_id *, int *);
