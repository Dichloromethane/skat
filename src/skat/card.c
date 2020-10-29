#include "skat/card.h"
#include "skat/game_rules.h"

const char *const CARD_TYPE_NAMES = "789DK0AB";
const uint8_t CARD_SCORES[] = {0, 0, 0, 3, 4, 10, 11, 2};
const uint8_t COMPARE_CARD_TYPE_NULL[] = {0, 1, 2, 5, 6, 3, 7, 4};
const char *const CARD_COLOR_NAMES = "KaHePiKr";

int
card_get(const card_id *const cid, card *const c) {
  card_type ct = *cid & 0b1111u;
  card_color cc = (*cid & 0b1110000u) >> 4u;

  if (cc <= COLOR_INVALID || cc > COLOR_KREUZ || ct <= CARD_TYPE_INVALID
	  || ct > CARD_TYPE_B)
	return 1;

  c->ct = ct;
  c->cc = cc;

  return 0;
}

int
card_get_id(const card *const c, card_id *const cid) {
  if (c->cc <= COLOR_INVALID || c->cc > COLOR_KREUZ
	  || c->ct <= CARD_TYPE_INVALID || c->ct > CARD_TYPE_B)
	return 1;

  *cid = (c->ct & 0b1111u) | ((c->cc & 0b111u) << 4u);
  return 0;
}

int
card_get_name(const card_id *const cid, char *const str) {
  card c;
  if (card_get(cid, &c))
	return 1;

  str[0] = CARD_COLOR_NAMES[2 * (c.cc - 1)];
  str[1] = CARD_COLOR_NAMES[(2 * (c.cc - 1)) + 1];
  str[2] = CARD_TYPE_NAMES[c.ct - 1];
  str[3] = '\0';

  return 0;
}

int
card_get_score(const card_id *const cid, uint8_t *const score) {
  card c;
  if (card_get(cid, &c))
	return 1;

  *score = CARD_SCORES[c.ct - 1];
  return 0;
}

static int
card_compare_id(const card_id *const cid1, const card_id *const cid2,
				const game_rules *const gr) {
  return (int) *cid1 - (int) *cid2;
}

static int
card_compare_hand(const card_id *const cid1, const card_id *const cid2,
				  const game_rules *const gr) {
  card c1, c2;
  if (gr->type == GAME_TYPE_INVALID || card_get(cid1, &c1)
	  || card_get(cid2, &c2))
	return 0;

  // color order
  if (gr->type == GAME_TYPE_NULL || gr->type == GAME_TYPE_RAMSCH) {
	if (c1.cc != c2.cc)
	  return (int) c1.cc - (int) c2.cc;
  } else {// GAME_TYPE_GRAND/GAME_TYPE_COLOR
	uint8_t adj_cc1;
	if (c1.ct == CARD_TYPE_B)
	  adj_cc1 = 6;
	else if (gr->type == GAME_TYPE_COLOR && c1.cc == gr->trumpf)
	  adj_cc1 = 5;
	else
	  adj_cc1 = c1.cc;

	uint8_t adj_cc2;
	if (c2.ct == CARD_TYPE_B)
	  adj_cc2 = 6;
	else if (gr->type == GAME_TYPE_COLOR && c2.cc == gr->trumpf)
	  adj_cc2 = 5;
	else
	  adj_cc2 = c2.cc;

	if (adj_cc1 != adj_cc2)
	  return (int) adj_cc1 - (int) adj_cc2;
  }

  // type order
  if (gr->type == GAME_TYPE_COLOR || gr->type == GAME_TYPE_GRAND
	  || gr->type == GAME_TYPE_RAMSCH)
	return (int) c1.ct - (int) c2.ct;
  else// GAME_TYPE_NULL
	return (int) COMPARE_CARD_TYPE_NULL[c1.ct - 1]
		   - (int) COMPARE_CARD_TYPE_NULL[c2.ct - 1];
}

static int
card_compare_stiche(const card_id *const cid1, const card_id *const cid2,
					const game_rules *const gr) {
  card c1, c2;
  if (gr->type == GAME_TYPE_INVALID || card_get(cid1, &c1)
	  || card_get(cid2, &c2))
	return 0;

  uint8_t score1, score2;
  if (card_get_score(cid1, &score1) || card_get_score(cid2, &score2))
	return 0;

  if (score1 != score2)
	return (int) score1 - (int) score2;

  return card_compare_hand(cid1, cid2, gr);
}

int
card_compare(const card_id *const cid1, const card_id *const cid2,
			 const card_compare_args *const args) {
  const game_rules *const gr = args->gr;
  const card_sort_mode *const mode = args->mode;

  if (*mode == CARD_SORT_MODE_ID)
	return card_compare_id(cid1, cid2, gr);
  else if (*mode == CARD_SORT_MODE_HAND)
	return card_compare_hand(cid1, cid2, gr);
  return card_compare_stiche(cid1, cid2, gr);
}
