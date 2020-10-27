#include "skat/player.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

int
player_equals_by_name(const player *const p1, const player *const p2) {
  return player_name_equals(&p1->name, &p2->name);
}

int
player_name_equals(const player_name *const p1, const player_name *const p2) {
  if (p1->length != p2->length)
	return 0;
  return !strncmp(p1->name, p2->name, p1->length);
}

void
copy_player_name(player_name *dest, const player_name *src) {
  assert(dest->length >= src->length);
  strncpy(dest->name, src->name, src->length);
  dest->name[src->length] = '\0';
  dest->length = src->length;
}

player_name *
create_player_name(const char *const str) {
  size_t length = strnlen(str, PLAYER_MAX_NAME_LENGTH - 1);
  player_name *ret = malloc(sizeof(player_name) + length + 1);
  ret->length = length;
  strncpy(ret->name, str, length);
  ret->name[length] = '\0';
  return ret;
}

size_t
player_name_extra_size(const player_name *const pname) {
  return pname->length + 1;
}

void
destroy_player_name(player_name *const pname) {
  pname->length = -1;
  free(pname);
}

player *
create_player(int gupid, int ap, const player_name *const pname) {
  size_t extra = player_name_extra_size(pname);
  player *ret = malloc(sizeof(player) + extra);
  ret->gupid = gupid;
  ret->ap = ap;
  ret->name.length = pname->length;
  copy_player_name(&ret->name, pname);
  return ret;
}
