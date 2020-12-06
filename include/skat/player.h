#pragma once

#include <stddef.h>
#include <stdint.h>

#define PLAYER_MAX_NAME_LENGTH 256

typedef struct {
  size_t name_length;
  int8_t gupid;// gupid, 0-3
  int8_t ap;   // active player index, -1 or 0-2
  char name[];
} player;

int player_equals_by_name(const player *p1, const player *p2);
player *create_player(int8_t gupid, int8_t ap, const char *constname);
