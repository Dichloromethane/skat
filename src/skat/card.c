#include "skat/card.h"

const char *CARD_TYPE_NAMES = "789BDK0A";
const uint8_t CARD_SCORES[] = {0, 0, 0, 2, 3, 4, 10, 11};
const char *CARD_COLOR_NAMES = "KaHePiKr";

int
card_get(const card_id *const cid, card *const c) {
  c->cc = (*cid & 0b11u) + 1;
  c->ct = ((*cid & 0b111100u) >> 2u) + 1;

  if (c->cc == COLOR_INVALID || c->ct == CARD_TYPE_INVALID)
	return 1;

  return 0;
}

int
card_get_id(const card *const c, card_id *const cid) {
  if (c->cc == COLOR_INVALID || c->ct == CARD_TYPE_INVALID)
	return 1;

  *cid = ((c->cc - 1) & 0b11u) | (((c->ct - 1) & 0b1111u) << 2u);
  return 0;
}

int
card_get_name(const card_id *const cid, char *const str) {
  card c;
  if (card_get(cid, &c) || c.cc == COLOR_INVALID || c.ct == CARD_TYPE_INVALID)
	return 1;

  str[0] = CARD_COLOR_NAMES[2 * (c.cc - 1)];
  str[1] = CARD_COLOR_NAMES[(2 * (c.cc - 1)) + 1];
  str[2] = CARD_TYPE_NAMES[c.ct - 1];
  str[3] = '\0';

  return 0;
}

int
card_get_score(const card_id *const cid, int *const score) {
  card c;
  if (card_get(cid, &c) || c.ct == CARD_TYPE_INVALID)
	return 1;

  *score = CARD_SCORES[c.ct - 1];
  return 0;
}
