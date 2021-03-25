#include "client/card_printer.h"
#include "skat/skat.h"
#include "skat/util.h"
#include <stdio.h>
#include <stdlib.h>

#define RED_CARD_COLOR     "\e[31;1m"
#define BLACK_CARD_COLOR   "\e[30;1m"
#define PLAYABLE_COLOR     "\e[4m"
#define NOT_PLAYABLE_COLOR ""

void
print_card_array(const shared_game_state *const sgs,
				 const card_collection *const cc, const card_id *const arr,
				 const size_t length, const card_color_mode color_mode) {
  char buf[4];
  card card;
  for (size_t i = 0; i < length; i++) {
	const card_id *const cid = &arr[i];

	int error = card_get(cid, &card);
	if (error)
	  continue;

	error = card_get_name(cid, buf);
	if (error)
	  continue;

	if (color_mode == CARD_COLOR_MODE_PLAYABLE) {
	  if (cc == NULL) {
		printf("Cannot determine if cards are playable without a card "
			   "collection\n");
		return;
	  }

	  int is_playable;
	  error = stich_card_legal(&sgs->gr, &sgs->curr_stich, cid, cc,
							   &is_playable);
	  if (error)
		continue;

	  printf(" %s%s%s(%d)" COLOR_CLEAR,
			 is_playable ? PLAYABLE_COLOR : NOT_PLAYABLE_COLOR,
			 (card.cc == COLOR_KREUZ || card.cc == COLOR_PIK) ? BLACK_CARD_COLOR
															  : RED_CARD_COLOR,
			 buf, *cid);
	} else if (color_mode == CARD_COLOR_MODE_ONLY_CARD_COLOR) {
	  printf(" %s%s(%d)" COLOR_CLEAR,
			 (card.cc == COLOR_KREUZ || card.cc == COLOR_PIK) ? BLACK_CARD_COLOR
															  : RED_CARD_COLOR,
			 buf, *cid);
	} else {
	  printf(" %s(%d)", buf, *cid);
	}
  }
}

void
print_card_collection(const shared_game_state *const sgs,
					  const card_collection *const cc,
					  const card_sort_mode sort_mode,
					  const card_color_mode color_mode) {
  uint8_t count;
  if (card_collection_get_card_count(cc, &count))
	return;

  card_id cid_array[count];// VLAaaaaaaaaaaaaaaaahhhhhhhhhhhh

  uint8_t j = 0;
  for (uint8_t i = 0; i < count; i++) {
	card_id cid;
	if (card_collection_get_card(cc, &i, &cid))
	  continue;

	cid_array[j++] = cid;
  }

  card_compare_args args =
		  (card_compare_args){.gr = &sgs->gr, .mode = &sort_mode};

  qsort_r(cid_array, j, sizeof(card_id),
		  (int (*)(const void *, const void *, void *)) card_compare, &args);

  print_card_array(sgs, cc, cid_array, j, color_mode);
}
