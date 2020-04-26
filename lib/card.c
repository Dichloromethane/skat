#include "card.h"
#include "skat.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

const char *CARD_TYPE_NAMES = "789BDK0A";
const char CARD_SCORES[] = {0, 0, 0, 2, 3, 4, 10, 11};
const char *CARD_COLOR_NAMES = "KaHePiKr";

int
rand_int(int min, int max) {
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
card_get(card_id *cid, card *c) {
  c->cc = (*cid & 3) + 1;
  c->ct = ((*cid >> 2) & 15) + 1;

  if (c->cc == COLOR_INVALID || c->ct == CARD_TYPE_INVALID) {
	return 1;
  }

  return 0;
}

int
card_get_id(card *c, card_id *cid) {
  if (c->cc == COLOR_INVALID || c->ct == CARD_TYPE_INVALID) {
	return 1;
  }

  *cid = ((c->cc - 1) & 3) | (((c->ct - 1) & 15) << 2);
  return 0;
}

int
card_get_name(card_id *cid, char *str) {
  card c;
  if (card_get(cid, &c)) {
	return 1;
  }

  if (c.cc == COLOR_INVALID || c.ct == CARD_TYPE_INVALID) {
	return 1;
  }

  str[0] = CARD_COLOR_NAMES[2 * (c.cc - 1)];
  str[1] = CARD_COLOR_NAMES[(2 * (c.cc - 1)) + 1];
  str[2] = CARD_TYPE_NAMES[c.ct - 1];
  str[3] = 0;

  return 0;
}

int
card_get_score(card_id *cid, int *score) {
  card c;
  if (card_get(cid, &c)) {
	return 1;
  }

  if (c.ct == CARD_TYPE_INVALID) {
	return 1;
  }

  *score = CARD_SCORES[c.ct - 1];
  return 0;
}

int
card_collection_contains(card_collection *col, card_id *cid, int *result) {
  *result = (*col >> *cid) & 1;
  return 0;
}

int
card_collection_add_card(card_collection *col, card_id *cid) {
  int result;
  card_collection_contains(col, cid, &result);
  if (result) {
	// card_collection already contains given card
	return 1;
  }

  *col |= 1 << *cid;
  return 0;
}

int
card_collection_add_card_array(card_collection *col, card_id *cid_array,
							   int array_size) {
  for (int i = 0; i < array_size; i++) {
	if (!card_collection_add_card(col, cid_array + i)) {
	  return 1;
	}
  }

  return 0;
}

int
card_collection_remove_card(card_collection *col, card_id *cid) {
  int result;
  card_collection_contains(col, cid, &result);
  if (!result) {
	// card_collection does not contain the given card
	return 1;
  }

  *col &= ~(1 << *cid);
  return 0;
}

int
card_collection_get_card_count(card_collection *col, int *count) {
  *count = __builtin_popcount(*col);
  return 0;
}

// XXX:
int
card_collection_get_card(card_collection *col, int idx, card_id *cid) {
  int result;
  int found = 0;

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
card_collection_get_score(card_collection *col, int *score) {
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
card_collection_empty(card_collection *col) {
  *col = 0;
  return 0;
}

int
card_collection_fill(card_collection *col) {
  *col = -1;
  return 0;
}

int
card_collection_draw_random(card_collection *col, card_id *cid) {
  int count;
  if (card_collection_get_card_count(col, &count) || count == 0) {
	return 1;
  }

  int i = rand_int(0, count);
  if (card_collection_get_card(col, i, cid)) {
	return 1;
  }

  return 0;
}

static unsigned int
stich_get_card_value(game_rules *gr, card *c0, card *c) {
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
stich_get_winner(game_rules *gr, stich *stich, int *result) {
  card c0, c1, c2;
  if (card_get(&stich->cs[0], &c0) || card_get(&stich->cs[1], &c1) ||
	  card_get(&stich->cs[2], &c2)) {
	return 1;
  }

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

static int
stich_is_trumpf(game_rules *gr, card_id cid) {
  if (gr->type == GAME_TYPE_NULL)
	return 0;

  card c;
  card_get(&cid, &c);
  int b = c.ct == CARD_TYPE_B;

  return b || (gr->type == GAME_TYPE_COLOR && c.cc == gr->trumpf);
}

static int
stich_bekennt(game_rules *gr, card_id first_id, card_id cid) {
  if (stich_is_trumpf(gr, first_id) && stich_is_trumpf(gr, cid))
	return 1;

  card first, c;
  card_get(&first_id, &first);
  card_get(&cid, &c);

  return first.cc == c.cc;
}

static int
stich_bekennt_any(game_rules *gr, card_id first_id, card_collection *hand) {
  int result;
  for (card_id cid = 0; cid < 32; cid++)
	if (!card_collection_contains(hand, &cid, &result) && result && stich_bekennt(gr, first_id, cid))
	  return 1;
  return 0;
}

int
stich_card_legal(game_rules *gr, card_id *played_cards,
				 card_id new_card, card_collection *hand,
				 int played_cards_size) {
  int result;
  if (card_collection_contains(hand, &new_card, &result) || !result)
	return 0;

  if (!played_cards_size)
	return 1;

  if (stich_bekennt(gr, played_cards[0], new_card))
	return 1;

  if (stich_bekennt_any(gr, played_cards[0], hand))
	return 0;

  return 1;
}
