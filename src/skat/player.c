#include "skat/player.h"
#include <string.h>

int
player_equals_by_name(const player *const p1, const player *const p2) {
  return player_name_equals(&p1->name, &p2->name);
}

int
player_name_equals(const player_name *const p1, const player_name *const p2) {
  return !strncmp(p1->str, p2->str, PLAYER_NAME_LENGTH);
}

void
copy_player_name(player_name *dest, const player_name *src) {
  strncpy(dest->str, src->str, PLAYER_NAME_LENGTH);
  dest->str[PLAYER_NAME_LENGTH - 1] = '\0';
}

void
init_player_name(player_name *dest, const char *str) {
  strncpy(dest->str, str, PLAYER_NAME_LENGTH);
  dest->str[PLAYER_NAME_LENGTH - 1] = '\0';
}
