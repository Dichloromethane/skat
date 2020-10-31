#pragma once

#include <stddef.h>

#define PLAYER_MAX_NAME_LENGTH 256

typedef struct {
  int gupid;// gupid, 0-3
  int ap;   // active player index, -1 or 0-2
  size_t name_length;
  char name[];
} player;

int player_equals_by_name(const player *p1, const player *p2);
player *create_player(int gupid, int ap, const char *name);
