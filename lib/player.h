
#pragma once

#define PLAYER_ID_LENGTH 16

typedef struct {
  char str[PLAYER_ID_LENGTH];
} player_id;

typedef struct {
  player_id id;
} player;

int player_id_equals(player_id *, player_id *);

