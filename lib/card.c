#include "card.h"
#include "skat.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

const char *CARD_TYPE_NAMES = "789BDK0A";
const char CARD_SCORES[] = {0, 0, 0, 2, 3, 4, 10, 11};
const char *CARD_COLOR_NAMES = "KaHePiKr";

static int
rand_int(const int min, const int max) {
  int fd = open("/dev/urandom", O_RDONLY);
  if (fd == -1) {
	perror("Error while accessing '/dev/urandom': ");
	exit(1);
  }

  int random;
  read(fd, &random, sizeof(int));
  close(fd);

  return (random % (max - min)) + min;
}


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
  str[3] = 0;

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

int
card_collection_contains(const card_collection *const col,
						 const card_id *const cid, int *const result) {
  *result = (int) ((*col >> *cid) & 0b1u);
  return 0;
}

int
card_collection_add_card(card_collection *const col, const card_id *const cid) {
  int result;
  if (card_collection_contains(col, cid, &result))
	return 1;
  if (result)
	return 2;

  *col |= 0b1u << *cid;
  return 0;
}

int
card_collection_add_card_array(card_collection *col,
							   const card_id *const cid_array,
							   const int array_size) {
  for (int i = 0; i < array_size; i++)
	if (!card_collection_add_card(col, cid_array + i))
	  return 1;

  return 0;
}

int
card_collection_remove_card(card_collection *col, const card_id *const cid) {
  int result;
  if (card_collection_contains(col, cid, &result))
	return 1;
  if (!result)
	return 2;

  *col &= ~(0b1u << *cid);
  return 0;
}

int
card_collection_get_card_count(const card_collection *const col,
							   int *const count) {
  *count = __builtin_popcount(*col);
  return 0;
}

// XXX:
int
card_collection_get_card(const card_collection *const col,
						 const unsigned int idx, card_id *const cid) {
  int result;
  unsigned int found = 0;

  for (card_id cur = 0; cur < 32; cur++) {
	card_collection_contains(col, &cur, &result);
	if (result && found++ == idx) {
	  *cid = cur;
	  return 0;
	}
  }

  return 1;
}

int
card_collection_get_score(const card_collection *const col, int *const score) {
  int card_score;
  int total_score = 0;
  int result;

  for (card_id cur = 0; cur < 32; cur++) {
	card_collection_contains(col, &cur, &result);
	if (result) {
	  card_get_score(&cur, &card_score);
	  total_score += card_score;
	}
  }

  *score = total_score;
  return 0;
}

int
card_collection_empty(card_collection *const col) {
  *col = 0;
  return 0;
}

int
card_collection_fill(card_collection *const col) {
  *col = -1;
  return 0;
}

int
card_collection_draw_random(const card_collection *const col,
							card_id *const cid) {
  int count;
  if (card_collection_get_card_count(col, &count) || count == 0)
	return 1;

  int i = rand_int(0, count);
  if (card_collection_get_card(col, i, cid))
	return 1;

  return 0;
}

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
