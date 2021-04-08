#include "skat/card_collection.h"
#include "skat/util.h"

static int
card_collection_index_from_id(const card_id *const cid,
							  uint8_t *const result_index) {
  card c;
  if (card_get(cid, &c))
	return 1;

  *result_index = ((c.ct - 1) & 0b111u) | (((c.cc - 1) & 0b11u) << 3u);

  return 0;
}

static int
card_collection_id_from_index(const uint8_t *const card_index,
							  card_id *const result_cid) {
  card c = (card){.ct = (card_type) ((*card_index & 0b111u) + 1),
				  .cc = (card_color) (((*card_index & 0b11000u) >> 3u) + 1)};
  return card_get_id(&c, result_cid);
}

static int
card_collection_contains_index(const card_collection *const col,
							   const uint8_t *const card_index,
							   int *const result) {
  *result = (int) ((*col >> *card_index) & 0b1u);
  return 0;
}

int
card_collection_contains(const card_collection *const col,
						 const card_id *const cid, int *const result) {
  uint8_t card_index;
  if (card_collection_index_from_id(cid, &card_index))
	return 1;

  return card_collection_contains_index(col, &card_index, result);
}

int
card_collection_add_card(card_collection *const col, const card_id *const cid) {
  uint8_t card_index;
  if (card_collection_index_from_id(cid, &card_index))
	return 1;

  int result;
  if (card_collection_contains_index(col, &card_index, &result)) {
	DERROR_PRINTF("Error while trying to check if %u is contained in %#x", *cid,
				  *col);
	return 2;
  } else if (result) {
	DERROR_PRINTF("Error while adding %u to %#x, as it is already exists in "
				  "the collection",
				  *cid, *col);
	return 3;
  }

  *col |= 0b1u << card_index;
  return 0;
}

int
card_collection_add_card_array(card_collection *col,
							   const card_id *const cid_array,
							   const size_t array_size) {
  card_collection tmp = *col;
  for (size_t i = 0; i < array_size; i++) {
	int error = card_collection_add_card(&tmp, &cid_array[i]);
	if (error) {
	  DERROR_PRINTF("Error %d while trying to add card %u to collection %#x",
					error, cid_array[i], tmp);
	  return 1;
	}
  }

  *col = tmp;
  return 0;
}

int
card_collection_remove_card(card_collection *col, const card_id *const cid) {
  uint8_t card_index;
  if (card_collection_index_from_id(cid, &card_index))
	return 1;

  int result;
  if (card_collection_contains_index(col, &card_index, &result))
	return 2;
  if (!result)
	return 3;

  *col &= ~(0b1u << card_index);
  return 0;
}

int
card_collection_remove_card_array(card_collection *col,
								  const card_id *cid_array, size_t array_size) {
  card_collection tmp = *col;
  for (size_t i = 0; i < array_size; i++) {
	int error = card_collection_remove_card(&tmp, &cid_array[i]);
	if (error) {
	  DERROR_PRINTF(
			  "Error %d while trying to remove card %u from collection %#x",
			  error, cid_array[i], tmp);
	  return 1;
	}
  }

  *col = tmp;
  return 0;
}

int
card_collection_get_card_count(const card_collection *const col,
							   uint8_t *const count) {
  *count = __builtin_popcount(*col);
  return 0;
}

// XXX: very inefficient
int
card_collection_get_card(const card_collection *const col,
						 const uint8_t *const idx, card_id *const result_cid) {
  unsigned int found = 0;

  for (uint8_t card_index = 0; card_index < 32; card_index++) {
	int result;
	int error = card_collection_contains_index(col, &card_index, &result);
	if (error)
	  return 1;

	if (result && found++ == *idx) {
	  error = card_collection_id_from_index(&card_index, result_cid);
	  if (error)
		return 2;
	  return 0;
	}
  }

  return 3;
}

int
card_collection_get_score(const card_collection *const col,
						  uint8_t *const score) {
  uint8_t total_score = 0;

  for (uint8_t card_index = 0; card_index < 32; card_index++) {
	int result;
	int error = card_collection_contains_index(col, &card_index, &result);
	if (error)
	  return 1;

	if (result) {
	  card_id cid;
	  error = card_collection_id_from_index(&card_index, &cid);
	  if (error)
		return 2;

	  uint8_t card_score;
	  card_get_score(&cid, &card_score);
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
card_collection_draw_random(card_collection const *const col,
							card_id *const cid) {
  uint8_t count;
  if (card_collection_get_card_count(col, &count) || count == 0) {
	DERROR_PRINTF("Could not draw random card: card collection is empty");
	return 1;
  }

  uint8_t i = (uint8_t) util_rand_int(0, count);
  if (card_collection_get_card(col, &i, cid)) {
	DERROR_PRINTF("Could not draw random card: randomly selected index %d does "
				  "not exist; count=%d, collection=%#x",
				  i, count, *col);
	return 2;
  }

  return 0;
}
