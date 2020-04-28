#include "skat/stich.h"

static unsigned int
stich_get_card_value(const game_rules *const gr, const card *const c0,
					 const card *const c) {
  switch (gr->type) {
	case GAME_TYPE_COLOR:
	case GAME_TYPE_GRAND:
	case GAME_TYPE_RAMSCH:
	  if (c->ct == CARD_TYPE_B) {
		return 20 + c->cc;
	  } else if (gr->type == GAME_TYPE_COLOR && c->cc == gr->trumpf) {
		return 10 + c->ct;
	  } else if (c->cc == c0->cc) {
		return c->ct;
	  } else {
		return 0;
	  }
	case GAME_TYPE_NULL:
	  if (c->cc == c0->cc) {
		return c->ct == CARD_TYPE_10 ? 7 : 2 * c->ct;
	  } else {
		return 0;
	  }
	default:
	  return 0;
  }
}

int
stich_get_winner(const game_rules *const gr, const stich *const stich,
				 int *const result) {
  card c0, c1, c2;
  if (card_get(&stich->cs[0], &c0) || card_get(&stich->cs[1], &c1)
	  || card_get(&stich->cs[2], &c2))
	return 1;

  unsigned int t0 = stich_get_card_value(gr, &c0, &c0);
  unsigned int t1 = stich_get_card_value(gr, &c0, &c1);
  unsigned int t2 = stich_get_card_value(gr, &c0, &c2);

  int winner = 0;

  if (t1 > t0) {
	winner = 1;
  }
  if (t2 > t0 || t2 > t1) {
	winner = 2;
  }

  *result = (stich->vorhand + winner) % 3;
  return 0;
}

static unsigned int
stich_is_trumpf(const game_rules *const gr, const card_id *const cid) {
  if (gr->type == GAME_TYPE_NULL)
	return 0;

  card c;
  card_get(cid, &c);
  int b = c.ct == CARD_TYPE_B;

  return b || (gr->type == GAME_TYPE_COLOR && c.cc == gr->trumpf);
}

static int
stich_bekennt(const game_rules *const gr, const card_id *const first_id,
			  const card_id *const cid) {
  if (stich_is_trumpf(gr, first_id) && stich_is_trumpf(gr, cid))
	return 1;

  card first, c;
  card_get(first_id, &first);
  card_get(cid, &c);

  return first.cc == c.cc;
}

static int
stich_bekennt_any(const game_rules *const gr, const card_id *const first_id,
				  const card_collection *const hand) {
  int result;
  for (card_id cid = 0; cid < 32; cid++)
	if (!card_collection_contains(hand, &cid, &result) && result
		&& stich_bekennt(gr, first_id, &cid))
	  return 1;
  return 0;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-branch-clone"
int
stich_card_legal(const game_rules *const gr, const card_id *const played_cards,
				 const int *const played_cards_size,
				 const card_id *const new_card,
				 const card_collection *const hand, int *const result) {
  int result_, contains;
  if (card_collection_contains(hand, new_card, &contains))
	return 1;
  else if (!contains)
	result_ = 0;
  else if (!*played_cards_size || stich_bekennt(gr, &played_cards[0], new_card))
	result_ = 1;
  else if (stich_bekennt_any(gr, &played_cards[0], hand))
	result_ = 0;
  else
	result_ = 1;

  *result = result_;
  return 0;
}
#pragma clang diagnostic pop