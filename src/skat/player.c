#include "skat/player.h"
#include <stdlib.h>
#include <string.h>

int
player_equals_by_name(const player *const p1, const player *const p2) {
  if (p1->name_length != p2->name_length)
	return 0;
  return !strncmp(p1->name, p2->name, p1->name_length);
}

player *
create_player(int gupid, int ap, const char *const name) {
  size_t name_length = strlen(name);
  player *pl = malloc(sizeof(player) + name_length + 1);
  pl->gupid = gupid;
  pl->ap = ap;
  pl->name_length = name_length;
  memcpy(pl->name, name, name_length + 1);
  return pl;
}
