#pragma once

#define _GNU_SOURCE

#define PLAYER_NAME_LENGTH 16

typedef struct {
  char str[PLAYER_NAME_LENGTH];
} player_name;

typedef struct {
  player_name name;
  int index;// gupid, 0-3
} player;

int player_equals_by_name(const player *p1, const player *p2);
int player_name_equals(const player_name *p1, const player_name *p2);
void copy_player_name(player_name *dest, const player_name *src);
void init_player_name(player_name *dest, const char *str);
