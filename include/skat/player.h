#pragma once

#define PLAYER_ID_LENGTH 16

typedef struct {
  char str[PLAYER_ID_LENGTH];
} player_id;

typedef struct {
  player_id id;
  int index;
} player;

int player_equals_by_id(const player *, const player *);
int player_id_equals(const player_id *, const player_id *);
void copy_player_id(player_id *, const player_id *);
void init_player_id(player_id *, const char *);
