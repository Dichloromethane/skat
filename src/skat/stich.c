#include "skat/stich.h"
#include "skat/util.h"

static unsigned int
stich_get_card_value(const game_rules *const gr, const card *const c0,
					 const card *const c) {
  switch (gr->type) {
	case GAME_TYPE_COLOR:
	case GAME_TYPE_GRAND:
	case GAME_TYPE_RAMSCH:
	  if (c->ct == CARD_TYPE_B)
		return 20 + c->cc;
	  else if (gr->type == GAME_TYPE_COLOR && c->cc == gr->trumpf)
		return 10 + c->ct;
	  else if (c->cc == c0->cc)
		return c->ct;
	  return 0;
	case GAME_TYPE_NULL:
	  if (c->cc == c0->cc) {
		if (c->ct == CARD_TYPE_10)
		  return 10;
		else if (c->ct == CARD_TYPE_B)
		  return 11;
		return 3 * c->ct;
	  }
	  return 0;
	default:
	  DERROR_PRINTF("Game Type is invalid");
	  exit(EXIT_FAILURE);
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
  if (t2 > t0 && t2 > t1) {
	winner = 2;
  }

  *result = winner;
  return 0;
}

static int
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
  int first_trumpf = stich_is_trumpf(gr, first_id);
  int trumpf = stich_is_trumpf(gr, cid);
  if (first_trumpf && trumpf)
	return 1;
  else if (first_trumpf || trumpf)
	return 0;

  card first, c;
  card_get(first_id, &first);
  card_get(cid, &c);

  return first.cc == c.cc;
}

static int
stich_bekennt_any(const game_rules *const gr, const card_id *const first_id,
				  const card_collection *const hand) {
  uint8_t count;
  if (card_collection_get_card_count(hand, &count))
	return 0;

  for (uint8_t i = 0; i < count; i++) {
	card_id cid;
	if (card_collection_get_card(hand, &i, &cid))
	  continue;

	if (stich_bekennt(gr, first_id, &cid))
	  return 1;
  }
  return 0;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-branch-clone"
int
stich_card_legal(const game_rules *const gr, const stich *const stich,
				 const card_id *const new_card,
				 const card_collection *const hand, int *const result) {
  int result_, contains;
  if (card_collection_contains(hand, new_card, &contains))
	return 1;// error while checking contains
  else if (!contains)
	result_ = 0;// cannot play card that you do not own
  else if (!stich->played_cards)
	result_ = 1;// first card can be anything
  else if (stich_bekennt(gr, &stich->cs[0], new_card))
	result_ = 1;// later cards have to be of the same color (or also a trumpf
				// card)
  else if (stich_bekennt_any(gr, &stich->cs[0], hand))
	result_ = 0;// you *have* to play a card with matching color
  else
	result_ = 1;// you can play anything if you cannot play a card with matching
				// color

  *result = result_;
  return 0;
}
#pragma clang diagnostic pop
