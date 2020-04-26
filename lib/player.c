#include"player.h"

int
player_id_equals(player *p1, player *p2) {
  return !strncmp(p1->id, p2->id, PLAYER_ID_LENGTH);
}
