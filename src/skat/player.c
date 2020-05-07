#include "skat/player.h"
#include <string.h>

int
player_equals_by_id(const player *const p1, const player *const p2) {
  return player_id_equals(&p1->id, &p2->id);
}

int
player_id_equals(const player_id *const p1, const player_id *const p2) {
  return !strncmp(p1->str, p2->str, PLAYER_ID_LENGTH);
}

void
copy_player_id(player_id *dest, const player_id *src) {
  strncpy(dest->str, src->str, PLAYER_ID_LENGTH);
  dest->str[PLAYER_ID_LENGTH - 1] = '\0';
}

void
init_player_id(player_id *dest, const char *str) {
  strncpy(dest->str, str, PLAYER_ID_LENGTH);
  dest->str[PLAYER_ID_LENGTH - 1] = '\0';
}
