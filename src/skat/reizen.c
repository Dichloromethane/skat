#include "skat/reizen.h"
#undef REIZEN_HDR
#define REIZ_PHASE_HDR_TO_STRING
#include "skat/reizen.h"

#include "skat/card.h"
#include "skat/skat.h"

#define REIZWERTE_LENGTH (63)
const uint16_t reizwerte[REIZWERTE_LENGTH] = {
		18,  20,  22,  23,  24,  27,  30,  33,  35,  36,  40,  44,  45,
		46,  48,  50,  54,  55,  59,  60,  63,  66,  70,  72,  77,  80,
		81,  84,  88,  90,  96,  99,  100, 108, 110, 117, 120, 121, 126,
		130, 132, 135, 140, 143, 144, 150, 153, 154, 156, 160, 162, 165,
		168, 170, 176, 180, 187, 192, 198, 204, 216, 240, 264};

const uint8_t grundwerte_color[4] = {9, 10, 11, 12};
const uint8_t grundwert_grand = 24;

const uint8_t spielwert_null = 23;
const uint8_t spielwert_null_hand = 35;
const uint8_t spielwert_null_ouvert = 46;
const uint8_t spielwert_null_hand_ouvert = 59;

uint16_t
reizen_get_next_reizwert(reiz_state *rs) {
  if (rs->rphase == REIZ_PHASE_INVALID || rs->rphase == REIZ_PHASE_DONE)
	return 0;

  uint16_t prev = 0, cur;
  for (size_t i = 0; i < REIZWERTE_LENGTH; i++) {
	cur = reizwerte[i];
	if (rs->reizwert >= prev && rs->reizwert < cur)
	  return cur;
	prev = cur;
  }

  return 0;
}

static int8_t
reizen_count_spitzen(const game_rules *const gr,
					 const card_collection *const initial_hand) {
  if (gr->type == GAME_TYPE_INVALID || gr->type == GAME_TYPE_NULL)
	return 0;

  card c = (card){.cc = COLOR_KREUZ, .ct = CARD_TYPE_B};
  card_id cid;
  int error = card_get_id(&c, &cid);
  if (error)
	return 0;

  int mit;
  error = card_collection_contains(initial_hand, &cid, &mit);
  if (error)
	return 0;

  int8_t spitzen = 1;

  for (card_color cc = COLOR_PIK; cc >= COLOR_KARO; cc--) {
	c.cc = cc;

	error = card_get_id(&c, &cid);
	if (error)
	  return 0;

	int result;
	error = card_collection_contains(initial_hand, &cid, &result);
	if (error)
	  return 0;

	if (result == mit)
	  spitzen++;
	else
	  goto end;
  }

  if (gr->type == GAME_TYPE_GRAND)
	goto end;

  c.cc = gr->trumpf;
  for (card_type ct = CARD_TYPE_A; ct >= CARD_TYPE_7; ct--) {
	c.ct = ct;

	error = card_get_id(&c, &cid);
	if (error)
	  return 0;

	int result;
	error = card_collection_contains(initial_hand, &cid, &result);
	if (error)
	  return 0;

	if (result == mit)
	  spitzen++;
	else
	  goto end;
  }

end:
  return mit ? spitzen : -spitzen;
}

// Do NOT use for ramschen
uint16_t
reizen_get_game_value(skat_server_state *ss, int won, int schneider,
					  int schwarz) {
  if (ss->sgs.gr.type == GAME_TYPE_INVALID
	  || ss->sgs.gr.type == GAME_TYPE_RAMSCH)
	return 0;

  if (ss->sgs.gr.type == GAME_TYPE_NULL) {
	if (ss->sgs.gr.hand) {
	  if (ss->sgs.gr.ouvert)
		return spielwert_null_hand_ouvert;
	  return spielwert_null_hand;
	} else if (ss->sgs.gr.ouvert)
	  return spielwert_null_ouvert;
	return spielwert_null;
  }

  uint64_t grundwert = ss->sgs.gr.type == GAME_TYPE_GRAND
							   ? grundwert_grand
							   : grundwerte_color[ss->sgs.gr.trumpf - 1];

  int8_t s_spitzen =
		  reizen_count_spitzen(&ss->sgs.gr, &ss->initial_alleinspieler_hand);
  int mit = s_spitzen > 0;
  uint64_t spitzen = !mit ? -s_spitzen : s_spitzen;

  // TODO: configure gewinnstufe for lost games in schneider
  uint64_t gewinnstufe = 0;
  gewinnstufe++;                               // spiel
  gewinnstufe += ss->sgs.gr.hand;              // hand
  gewinnstufe += ss->sgs.gr.schneider_angesagt;// schneider angesagt
  gewinnstufe += ss->sgs.gr.schwarz_angesagt;  // schwarz angesagt
  gewinnstufe += ss->sgs.gr.ouvert;            // ouvert

  if (won) {
	if (schneider)
	  gewinnstufe++;// schneider
	if (schwarz)
	  gewinnstufe++;// schwarz
  }

  return (gewinnstufe + spitzen) * grundwert;
}
