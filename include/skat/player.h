#pragma once

#define _GNU_SOURCE

#include <stddef.h>

#define PLAYER_MAX_NAME_LENGTH 256

typedef struct {
  size_t length;
  char name[];
} player_name;

typedef struct {
  int index;// gupid, 0-3
  player_name name;
} player;

int player_equals_by_name(const player *p1, const player *p2);
int player_name_equals(const player_name *p1, const player_name *p2);
void copy_player_name(player_name *dest, const player_name *src);
player_name *create_player_name(const char *str);
size_t player_name_extra_size(const player_name *pname);
void destroy_player_name(player_name *pname);
player *create_player(int index, const player_name *pname);
