#include "player.h"

int
player_id_equals(player_id *p1, player_id *p2) {
  return !strncmp(&p1->id, &p2->id, PLAYER_ID_LENGTH);
}
