#include "card_collection.h"
#include "util.h"

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

  int i = util_rand_int(0, count);
  if (card_collection_get_card(col, i, cid))
	return 1;

  return 0;
}