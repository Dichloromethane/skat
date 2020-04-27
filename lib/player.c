#include "player.h"
#include <string.h>

int
		player_equals_by_id(const player *const p1, const player *const p2) {
  return player_id_equals(&p1->id, &p2->id);
}

int
		player_id_equals(const player_id *const p1, const player_id *const p2) {
  return !strncmp(p1->str, p2->str, PLAYER_ID_LENGTH);
}
