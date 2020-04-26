#include "player.h"
#include <string.h>

int
player_id_equals(player_id *p1, player_id *p2) {
  return !strncmp(p1->str, p2->str, PLAYER_ID_LENGTH);
}
