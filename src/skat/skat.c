#include "skat/skat.h"
#include "skat/card_collection.h"
#include "skat/client.h"
#include "skat/game_rules.h"
#include "skat/server.h"
#include "skat/util.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#undef SKAT_HDR
#define GAME_PHASE_HDR_TO_STRING

#include "skat/skat.h"

void
skat_state_notify_disconnect(skat_server_state *ss, player *pl, server *s) {
  DTODO_PRINTF("TODO: implement notify_disconnect");// TODO: implement
}

void
skat_state_notify_join(skat_server_state *ss, player *pl, server *s) {
  DTODO_PRINTF("TODO: implement notify_join");// TODO: implement
}

static int
get_score_delta_normal_game(game_rules *gs, reiz_state *rs, uint64_t game_value,
							bool *won, loss_type *lt) {
  int grundwert;
  // TODO: Make sense of the skat rules
  if (!*won) {
	*lt = LOSS_TYPE_LOST;
	return -2 * game_value;
  } else if (rs->reizwert > game_value) {
	grundwert = reizen_get_grundwert(gs);
	*won = false;
	*lt = LOSS_TYPE_LOST_UEBERREIZT;
	return -2 * ceil_div(rs->reizwert, grundwert) * grundwert;
  }

  *lt = LOSS_TYPE_WON;
  return game_value;
}

void
skat_calculate_game_result(skat_server_state *ss, round_result *rr) {
  uint64_t game_value;
  loss_type lt = LOSS_TYPE_INVALID;
  int as = ss->sgs.alleinspieler;
  bool won, schneider, schwarz, durchmarsch;

  rr->normal_end = ss->sgs.stich_num == 9;
  for (int i = 0; i < 3; i++) {
	card_collection_get_score(ss->stiche[i], &rr->player_points[i]);

	card_collection_get_card_count(ss->stiche[i],
								   &rr->player_stich_card_count[i]);
  }

  switch (ss->sgs.gr.type) {
	case GAME_TYPE_NULL:
	  won = rr->player_stich_card_count[as] == 0;
	  game_value = reizen_get_game_value(ss, won, false, false);
	  rr->spielwert = game_value;
	  rr->schneider = 0;
	  rr->schwarz = 0;
	  memset(rr->round_score, '\0', sizeof rr->round_score);
	  rr->round_score[as] = get_score_delta_normal_game(
			  &ss->sgs.gr, &ss->sgs.rs, game_value, &won, &lt);
	  rr->round_winner = won ? as : -1;
	  rr->loss_type = lt;
	  break;
	case GAME_TYPE_GRAND:
	case GAME_TYPE_COLOR:
	  (void) 0;
	  unsigned int as_points = rr->player_points[as];
	  if ((schneider = as_points >= 90)) {
		schwarz = rr->player_stich_card_count[as] == 32;
	  } else {
		schwarz = false;
	  }
	  won = as_points > 60 && (schneider || !ss->sgs.gr.schneider_angesagt)
			&& (schwarz || !ss->sgs.gr.schneider_angesagt);
	  game_value = reizen_get_game_value(ss, won, schneider, schwarz);
	  rr->spielwert = game_value;
	  rr->schneider = schneider;
	  rr->schwarz = schwarz;
	  memset(rr->round_score, '\0', sizeof rr->round_score);
	  rr->round_score[as] = get_score_delta_normal_game(
			  &ss->sgs.gr, &ss->sgs.rs, game_value, &won, &lt);
	  rr->round_winner = won ? as : -1;
	  rr->loss_type = lt;
	  break;
	case GAME_TYPE_RAMSCH:
	  memset(rr->round_score, '\0', sizeof rr->round_score);
	  rr->spielwert = -1;
	  rr->schneider = 0;
	  rr->schwarz = 0;
	  rr->round_winner = -1;
	  durchmarsch = false;
	  lt = LOSS_TYPE_RAMSCH;
	  for (int i = 0; i < 3; i++) {
		// The two cards in the skat don't count
		if (rr->player_stich_card_count[i] == 30) {
		  durchmarsch = true;
		  rr->round_winner = i;
		  rr->round_score[i] = 120;
		  lt = LOSS_TYPE_WON_DURCHMARSCH;
		  break;
		}
	  }
	  rr->loss_type = lt;
	  if (durchmarsch)
		break;
	  for (int i = 0; i < 3; i++) {
		// TODO: needs more virgins
		//   Also add configuration options for ramsch rules
		rr->round_score[i] = -rr->player_points[i];
	  }
	  break;
	default:
	  __builtin_unreachable();
  }
}

static void
get_player_hand(skat_server_state *ss, player *pl, card_collection *col) {
  if (pl->ap >= 0) {
	*col = ss->player_hands[pl->ap];
	return;
  }

  card_collection_empty(col);
}

#define RANDOM_CARD_DISTRIBUTE(draw_pile, player_hand) \
  do { \
	card_id cid_; \
	int error_; \
\
	error_ = card_collection_draw_random(draw_pile, &cid_); \
	if (error_) { \
	  DERROR_PRINTF("Error %d while drawing random card from draw pile %#x", \
					error_, *(draw_pile)); \
	} \
\
	error_ = card_collection_add_card(player_hand, &cid_); \
	if (error_) { \
	  DERROR_PRINTF("Error %d while adding card %u to collection %#x", error_, \
					cid_, *(draw_pile)); \
	} \
\
	error_ = card_collection_remove_card(draw_pile, &cid_); \
	if (error_) { \
	  DERROR_PRINTF("Error %d while removing card %u from collection %#x", \
					error_, cid_, *(draw_pile)); \
	} \
\
  } while (0)

// Conforming to the rules. Poggers.
static int
distribute_cards(skat_server_state *ss) {
  card_collection draw_pile;
  card_collection_fill(&draw_pile);

  for (int i = 0; i < 3; i++) {
	for (int j = 0; j < 3; j++) {
	  RANDOM_CARD_DISTRIBUTE(&draw_pile, &ss->player_hands[i]);
	}
  }

  for (int i = 0; i < 2; i++) {
	card_id cid_;
	int error_;

	error_ = card_collection_draw_random(&draw_pile, &cid_);
	if (error_) {
	  DERROR_PRINTF("Error %d while drawing random card from draw pile %#x",
					error_, draw_pile);
	}

	ss->skat[i] = cid_;

	error_ = card_collection_remove_card(&draw_pile, &cid_);
	if (error_) {
	  DERROR_PRINTF("Error %d while removing card %u from draw pile %#x",
					error_, cid_, draw_pile);
	}
  }

  for (int i = 0; i < 3; i++) {
	for (int j = 0; j < 4; j++) {
	  RANDOM_CARD_DISTRIBUTE(&draw_pile, &ss->player_hands[i]);
	}
  }

  for (int i = 0; i < 3; i++) {
	for (int j = 0; j < 3; j++) {
	  RANDOM_CARD_DISTRIBUTE(&draw_pile, &ss->player_hands[i]);
	}
  }

  return 0;
}

#if defined(DISTRIBUTE_SORTED_CARDS) && DISTRIBUTE_SORTED_CARDS
static int
debug_distribute_cards(skat_server_state *ss) {
  card_get_id(&(card){.cc = COLOR_KARO, .ct = CARD_TYPE_7}, &ss->skat[0]);
  card_get_id(&(card){.cc = COLOR_KARO, .ct = CARD_TYPE_8}, &ss->skat[1]);

  card_id hand[10];

  card_get_id(&(card){.cc = COLOR_KARO, .ct = CARD_TYPE_9}, &hand[0]);
  card_get_id(&(card){.cc = COLOR_KARO, .ct = CARD_TYPE_D}, &hand[1]);
  card_get_id(&(card){.cc = COLOR_KARO, .ct = CARD_TYPE_K}, &hand[2]);
  card_get_id(&(card){.cc = COLOR_KARO, .ct = CARD_TYPE_10}, &hand[3]);
  card_get_id(&(card){.cc = COLOR_KARO, .ct = CARD_TYPE_A}, &hand[4]);
  card_get_id(&(card){.cc = COLOR_HERZ, .ct = CARD_TYPE_7}, &hand[5]);
  card_get_id(&(card){.cc = COLOR_HERZ, .ct = CARD_TYPE_8}, &hand[6]);
  card_get_id(&(card){.cc = COLOR_HERZ, .ct = CARD_TYPE_9}, &hand[7]);
  card_get_id(&(card){.cc = COLOR_HERZ, .ct = CARD_TYPE_D}, &hand[8]);
  card_get_id(&(card){.cc = COLOR_HERZ, .ct = CARD_TYPE_K}, &hand[9]);
  card_collection_add_card_array(&ss->player_hands[0], hand, 10);

  card_get_id(&(card){.cc = COLOR_HERZ, .ct = CARD_TYPE_10}, &hand[0]);
  card_get_id(&(card){.cc = COLOR_HERZ, .ct = CARD_TYPE_A}, &hand[1]);
  card_get_id(&(card){.cc = COLOR_PIK, .ct = CARD_TYPE_7}, &hand[2]);
  card_get_id(&(card){.cc = COLOR_PIK, .ct = CARD_TYPE_8}, &hand[3]);
  card_get_id(&(card){.cc = COLOR_PIK, .ct = CARD_TYPE_9}, &hand[4]);
  card_get_id(&(card){.cc = COLOR_PIK, .ct = CARD_TYPE_D}, &hand[5]);
  card_get_id(&(card){.cc = COLOR_PIK, .ct = CARD_TYPE_K}, &hand[6]);
  card_get_id(&(card){.cc = COLOR_PIK, .ct = CARD_TYPE_10}, &hand[7]);
  card_get_id(&(card){.cc = COLOR_PIK, .ct = CARD_TYPE_A}, &hand[8]);
  card_get_id(&(card){.cc = COLOR_KREUZ, .ct = CARD_TYPE_7}, &hand[9]);
  card_collection_add_card_array(&ss->player_hands[1], hand, 10);

  card_get_id(&(card){.cc = COLOR_KREUZ, .ct = CARD_TYPE_8}, &hand[0]);
  card_get_id(&(card){.cc = COLOR_KREUZ, .ct = CARD_TYPE_9}, &hand[1]);
  card_get_id(&(card){.cc = COLOR_KREUZ, .ct = CARD_TYPE_D}, &hand[2]);
  card_get_id(&(card){.cc = COLOR_KREUZ, .ct = CARD_TYPE_K}, &hand[3]);
  card_get_id(&(card){.cc = COLOR_KREUZ, .ct = CARD_TYPE_10}, &hand[4]);
  card_get_id(&(card){.cc = COLOR_KREUZ, .ct = CARD_TYPE_A}, &hand[5]);
  card_get_id(&(card){.cc = COLOR_KARO, .ct = CARD_TYPE_B}, &hand[6]);
  card_get_id(&(card){.cc = COLOR_HERZ, .ct = CARD_TYPE_B}, &hand[7]);
  card_get_id(&(card){.cc = COLOR_PIK, .ct = CARD_TYPE_B}, &hand[8]);
  card_get_id(&(card){.cc = COLOR_KREUZ, .ct = CARD_TYPE_B}, &hand[9]);
  card_collection_add_card_array(&ss->player_hands[2], hand, 10);

  return 0;
}
#endif

static game_phase
apply_action_setup(skat_server_state *ss, action *a, player *pl, server *s) {
  event e;
  e.answer_to = a->id;
  e.acting_player = pl->gupid;
  switch (a->type) {
	case ACTION_READY:
	  if (s->ncons < 3) {
		DEBUG_PRINTF("Rejecting action ACTION_READY with id %ld by player %s "
					 "because s->ncons = %d < 3",
					 a->id, pl->name, s->ncons);
		return GAME_PHASE_INVALID;
	  }

	  e.type = EVENT_START_GAME;
	  server_distribute_event(s, &e, NULL);

	  return GAME_PHASE_BETWEEN_ROUNDS;
	default:

	  DEBUG_PRINTF("Trying to use undefined action %s in state %s",
				   action_name_table[a->type],
				   game_phase_name_table[ss->sgs.cgphase]);
	  return GAME_PHASE_INVALID;
  }
}

static game_phase
apply_action_between_rounds(skat_server_state *ss, action *a, player *pl,
							server *s) {
  int pm, ix;
  event e;
  e.answer_to = a->id;
  e.acting_player = pl->gupid;
  switch (a->type) {
	case ACTION_READY:
	  if (s->ncons < 3) {
		DEBUG_PRINTF("Rejecting action ACTION_READY with id %ld by player %s "
					 "because s->ncons = %d < 3",
					 a->id, pl->name, s->ncons);

		return GAME_PHASE_INVALID;
	  }

	  e.answer_to = a->id;
	  e.acting_player = pl->gupid;
	  e.type = EVENT_START_ROUND;

	  if (ss->sgs.active_players[0] == -1) {// first round
		for (int gupid = 0, ap = 0; ap < 3; gupid++)
		  if (server_is_player_active(s, gupid))
			ss->sgs.active_players[ap++] = s->pls[gupid]->gupid;
	  } else if (s->ncons == 3) {// we don't have a spectator
		perm(ss->sgs.active_players, 3, 0x12);
	  } else {// we have a spectator
		pm = 0;
		for (int ap = 0; ap < 3; ap++)
		  pm |= 1 << ss->sgs.active_players[ap];
		ix = __builtin_ctz(~pm);
		perm(ss->sgs.active_players, 3, 0x12);
		ss->sgs.active_players[2] = s->pls[ix]->gupid;
	  }

	  for (int gupid = 0; gupid < 4; gupid++) {
		if (s->pls[gupid])
		  s->pls[gupid]->ap = -1;
	  }

	  for (int ap = 0; ap < 3; ap++) {
		s->pls[ss->sgs.active_players[ap]]->ap = ap;
	  }

	  memcpy(e.current_active_players, ss->sgs.active_players,
			 sizeof(e.current_active_players));

	  server_distribute_event(s, &e, NULL);

	  ss->sgs.curr_stich =
			  (stich){.played_cards = 0, .vorhand = 0, .winner = -1};
	  ss->sgs.last_stich =
			  (stich){.played_cards = 0, .vorhand = -1, .winner = -1};
	  ss->sgs.stich_num = 0;
	  ss->sgs.alleinspieler = -1;
	  ss->sgs.rs.rphase = REIZ_PHASE_INVALID;
	  ss->sgs.rs.waiting_teller = -1;
	  ss->sgs.rs.reizwert = 0;
	  ss->sgs.rs.winner = -1;
	  memset(ss->stiche, '\0', 3 * sizeof(ss->stiche[0]));
	  card_collection_empty(&ss->stiche_buf[0]);
	  card_collection_empty(&ss->stiche_buf[1]);
	  card_collection_empty(&ss->stiche_buf[2]);

	  card_collection_empty(&ss->player_hands[0]);
	  card_collection_empty(&ss->player_hands[1]);
	  card_collection_empty(&ss->player_hands[2]);

	  card_collection_empty(&ss->initial_alleinspieler_hand);
	  card_collection_empty(&ss->initial_skat);
	  card_collection_empty(&ss->initial_alleinspieler_hand_with_skat);

#if defined(DISTRIBUTE_SORTED_CARDS) && (DISTRIBUTE_SORTED_CARDS)
	  // distributing manually for debugging:
	  debug_distribute_cards(ss);
#else
	  distribute_cards(ss);
#endif

	  DEBUG_PRINTF("Player hands: %#x, %#x, %#x", ss->player_hands[0],
				   ss->player_hands[1], ss->player_hands[2]);
	  DEBUG_PRINTF("Skat: %u & %u", ss->skat[0], ss->skat[1]);

	  e.type = EVENT_DISTRIBUTE_CARDS;
	  e.answer_to = -1;
	  e.acting_player = -1;

	  void mask_hands(event * ev, player * pl) {
		get_player_hand(ss, pl, &ev->hand);
	  }

	  server_distribute_event(s, &e, mask_hands);

	  ss->sgs.rs.rphase = REIZ_PHASE_MITTELHAND_TO_VORHAND;
	  ss->sgs.rs.waiting_teller = 1;
	  ss->sgs.rs.reizwert = 0;
	  ss->sgs.rs.winner = -1;

	  return GAME_PHASE_REIZEN;
	default:
	  DEBUG_PRINTF("Trying to use undefined action %s in state %s",
				   action_name_table[a->type],
				   game_phase_name_table[ss->sgs.cgphase]);
	  return GAME_PHASE_INVALID;
  }
}

static const uint8_t skat_stiche_buf_lookup[3][3] = {{0, 1, 1},
													 {0, 1, 0},
													 {0, 0, 1}};

static game_phase
finish_reizen(skat_server_state *ss, server *s, event *e) {
  e->answer_to = -1;
  e->acting_player = -1;
  e->type = EVENT_REIZEN_DONE;

  ss->sgs.rs.rphase = REIZ_PHASE_DONE;
  ss->sgs.rs.waiting_teller = 1;

  if (ss->sgs.rs.reizwert < 18) {
	ss->sgs.rs.reizwert = 0;

	ss->sgs.gr.type = GAME_TYPE_RAMSCH;

	ss->sgs.alleinspieler = -1;

	for (int ap = 0; ap < 3; ap++)
	  ss->stiche[ap] = &ss->stiche_buf[ap];

	e->alleinspieler = ss->sgs.alleinspieler;
	e->reizwert_final = ss->sgs.rs.reizwert;

	server_distribute_event(s, e, NULL);

	// TODO: implement schieberamsch
	return GAME_PHASE_PLAY_STICH_C1;
  } else {
	ss->sgs.alleinspieler = ss->sgs.rs.winner;

	ss->initial_alleinspieler_hand = ss->initial_alleinspieler_hand_with_skat =
			ss->player_hands[ss->sgs.alleinspieler];
	card_collection_add_card_array(&ss->initial_skat, ss->skat, 2);
	card_collection_add_card_array(&ss->initial_alleinspieler_hand_with_skat,
								   ss->skat, 2);

	for (int ap = 0; ap < 3; ap++)
	  ss->stiche[ap] =
			  &ss->stiche_buf[skat_stiche_buf_lookup[ss->sgs.alleinspieler]
													[ap]];

	e->alleinspieler = ss->sgs.alleinspieler;
	e->reizwert_final = ss->sgs.rs.reizwert;

	server_distribute_event(s, e, NULL);

	return GAME_PHASE_SKAT_AUFNEHMEN;
  }
}

static game_phase
apply_action_reizen(skat_server_state *ss, action *a, player *pl, server *s) {
  if (ss->sgs.rs.rphase == REIZ_PHASE_INVALID
	  || ss->sgs.rs.rphase == REIZ_PHASE_DONE) {
	DEBUG_PRINTF("Invalid reiz phase %s",
				 reiz_phase_name_table[ss->sgs.rs.rphase]);
	return GAME_PHASE_INVALID;
  }

  event e;
  e.answer_to = a->id;
  e.acting_player = pl->gupid;
  e.reizwert = 0;
  switch (a->type) {
	case ACTION_REIZEN_NUMBER:
	  if (!ss->sgs.rs.waiting_teller) {
		DEBUG_PRINTF("Not waiting for teller");
		return GAME_PHASE_INVALID;
	  }

	  if (ss->sgs.rs.rphase == REIZ_PHASE_MITTELHAND_TO_VORHAND
		  && pl->ap != 1) {
		DEBUG_PRINTF("Wrong player trying reizen number: expected 1 but got %d",
					 pl->ap);
		return GAME_PHASE_INVALID;
	  }

	  if (ss->sgs.rs.rphase == REIZ_PHASE_HINTERHAND_TO_WINNER && pl->ap != 2) {
		DEBUG_PRINTF("Wrong player trying reizen number: expected 2 but got %d",
					 pl->ap);
		return GAME_PHASE_INVALID;
	  }

	  if (ss->sgs.rs.rphase == REIZ_PHASE_WINNER
		  && pl->ap != ss->sgs.rs.winner) {
		DEBUG_PRINTF(
				"Wrong player trying reizen number: expected %d but got %d",
				ss->sgs.rs.winner, pl->ap);
		return GAME_PHASE_INVALID;
	  }

	  if (a->reizwert < 18 || a->reizwert <= ss->sgs.rs.reizwert) {
		DEBUG_PRINTF("Invalid reizwert %u for current reizwert %u", a->reizwert,
					 ss->sgs.rs.reizwert);
		return GAME_PHASE_INVALID;
	  }

	  ss->sgs.rs.reizwert = a->reizwert;
	  ss->sgs.rs.waiting_teller = 0;

	  e.type = EVENT_REIZEN_NUMBER;
	  e.reizwert = ss->sgs.rs.reizwert;

	  server_distribute_event(s, &e, NULL);

	  if (ss->sgs.rs.rphase == REIZ_PHASE_WINNER)
		return finish_reizen(ss, s, &e);

	  return GAME_PHASE_REIZEN;
	case ACTION_REIZEN_CONFIRM:
	  if ((ss->sgs.rs.rphase == REIZ_PHASE_MITTELHAND_TO_VORHAND
		   || ss->sgs.rs.rphase == REIZ_PHASE_HINTERHAND_TO_WINNER)
		  && ss->sgs.rs.waiting_teller) {
		DEBUG_PRINTF("Only the listener may confirm now");
		return GAME_PHASE_INVALID;
	  }

	  if (ss->sgs.rs.rphase == REIZ_PHASE_MITTELHAND_TO_VORHAND
		  && pl->ap != 0) {
		DEBUG_PRINTF(
				"Wrong player trying reizen confirm: expected 0 but got %d",
				pl->ap);
		return GAME_PHASE_INVALID;
	  }

	  if (ss->sgs.rs.rphase == REIZ_PHASE_HINTERHAND_TO_WINNER
		  && pl->ap != ss->sgs.rs.winner) {
		DEBUG_PRINTF(
				"Wrong player trying reizen confirm: expected %d but got %d",
				ss->sgs.rs.winner, pl->ap);
		return GAME_PHASE_INVALID;
	  }

	  ss->sgs.rs.waiting_teller = 1;

	  e.type = EVENT_REIZEN_CONFIRM;

	  server_distribute_event(s, &e, NULL);

	  if (ss->sgs.rs.rphase == REIZ_PHASE_WINNER) {
		if (ss->sgs.rs.reizwert < 18)
		  ss->sgs.rs.reizwert = 18;
		return finish_reizen(ss, s, &e);
	  }

	  return GAME_PHASE_REIZEN;
	case ACTION_REIZEN_PASSE:
	  if (ss->sgs.rs.rphase == REIZ_PHASE_MITTELHAND_TO_VORHAND && pl->ap != 1
		  && ss->sgs.rs.waiting_teller) {
		DEBUG_PRINTF("Wrong player trying reizen passe: expected 1 but got %d",
					 pl->ap);
		return GAME_PHASE_INVALID;
	  }

	  if (ss->sgs.rs.rphase == REIZ_PHASE_MITTELHAND_TO_VORHAND && pl->ap != 0
		  && !ss->sgs.rs.waiting_teller) {
		DEBUG_PRINTF("Wrong player trying reizen passe: expected 0 but got %d",
					 pl->ap);
		return GAME_PHASE_INVALID;
	  }

	  if (ss->sgs.rs.rphase == REIZ_PHASE_HINTERHAND_TO_WINNER && pl->ap != 2
		  && ss->sgs.rs.waiting_teller) {
		DEBUG_PRINTF("Wrong player trying reizen passe: expected 2 but got %d",
					 pl->ap);
		return GAME_PHASE_INVALID;
	  }

	  if (ss->sgs.rs.rphase == REIZ_PHASE_HINTERHAND_TO_WINNER
		  && pl->ap != ss->sgs.rs.winner && !ss->sgs.rs.waiting_teller) {
		DEBUG_PRINTF("Wrong player trying reizen passe: expected %d but got %d",
					 ss->sgs.rs.winner, pl->ap);
		return GAME_PHASE_INVALID;
	  }

	  e.type = EVENT_REIZEN_PASSE;

	  server_distribute_event(s, &e, NULL);

	  if (ss->sgs.rs.rphase == REIZ_PHASE_MITTELHAND_TO_VORHAND) {
		ss->sgs.rs.rphase = REIZ_PHASE_HINTERHAND_TO_WINNER;
		ss->sgs.rs.winner = !ss->sgs.rs.waiting_teller;
		ss->sgs.rs.waiting_teller = 1;
		return GAME_PHASE_REIZEN;
	  } else if (ss->sgs.rs.rphase == REIZ_PHASE_HINTERHAND_TO_WINNER) {
		if (!ss->sgs.rs.waiting_teller)
		  ss->sgs.rs.winner = 2;
		ss->sgs.rs.waiting_teller = 1;

		if (ss->sgs.rs.reizwert >= 18)
		  return finish_reizen(ss, s, &e);

		ss->sgs.rs.rphase = REIZ_PHASE_WINNER;
		return GAME_PHASE_REIZEN;
	  }
	  // REIZ_PHASE_WINNER
	  return finish_reizen(ss, s, &e);
	default:
	  DEBUG_PRINTF("Trying to use undefined action %s in state %s",
				   action_name_table[a->type],
				   game_phase_name_table[ss->sgs.cgphase]);
	  return GAME_PHASE_INVALID;
  }
}

static game_phase
apply_action_skat_aufnehmen(skat_server_state *ss, action *a, player *pl,
							server *s) {
  if (ss->sgs.alleinspieler == -1 || ss->sgs.alleinspieler != pl->ap) {
	DEBUG_PRINTF("Invalid skat actor");
	return GAME_PHASE_INVALID;
  }

  event e;
  e.answer_to = a->id;
  e.acting_player = pl->gupid;
  switch (a->type) {
	case ACTION_SKAT_TAKE:
	  ss->sgs.took_skat = 1;
	  card_collection_add_card_array(&ss->player_hands[ss->sgs.alleinspieler],
									 ss->skat, 2);

	  e.type = EVENT_SKAT_TAKE;
	  memset(e.skat, '\0', sizeof(e.skat));

	  void mask_skat(event * ev, player * pl) {
		if (ss->sgs.alleinspieler == pl->ap)
		  memcpy(ev->skat, ss->skat, sizeof(ev->skat));
	  }

	  server_distribute_event(s, &e, mask_skat);

	  return GAME_PHASE_SKAT_AUFNEHMEN;
	case ACTION_SKAT_LEAVE:
	  ss->sgs.took_skat = 0;
	  if (ss->sgs.gr.type != GAME_TYPE_NULL) {
		card_collection_add_card_array(&ss->stiche_buf[ss->sgs.alleinspieler],
									   ss->skat, 2);
	  }

	  e.type = EVENT_SKAT_LEAVE;

	  server_distribute_event(s, &e, NULL);

	  return GAME_PHASE_SPIELANSAGE;
	case ACTION_SKAT_PRESS:
	  (void) 0;// label hack to create a local variable
	  card_collection tmp = ss->player_hands[ss->sgs.alleinspieler];
	  if (card_collection_remove_card_array(&tmp, a->skat_press_cards, 2)) {
		DEBUG_PRINTF("Cannot press cards %d & %d", a->skat_press_cards[0],
					 a->skat_press_cards[1]);
		return GAME_PHASE_INVALID;
	  }

	  if (ss->sgs.gr.type != GAME_TYPE_NULL) {
		card_collection_add_card_array(&ss->stiche_buf[ss->sgs.alleinspieler],
									   a->skat_press_cards, 2);
	  }

	  ss->player_hands[ss->sgs.alleinspieler] = tmp;

	  e.type = EVENT_SKAT_PRESS;

	  memset(e.skat, '\0', sizeof(e.skat_press_cards));

	  void mask_skat_press_cards(event * ev, player * pl) {
		if (ss->sgs.alleinspieler == pl->ap)
		  memcpy(ev->skat_press_cards, a->skat_press_cards,
				 sizeof(ev->skat_press_cards));
	  }

	  server_distribute_event(s, &e, mask_skat_press_cards);

	  return GAME_PHASE_SPIELANSAGE;
	default:
	  DEBUG_PRINTF("Trying to use undefined action %s in state %s",
				   action_name_table[a->type],
				   game_phase_name_table[ss->sgs.cgphase]);
	  return GAME_PHASE_INVALID;
  }
}

static game_phase
apply_action_spielansage(skat_server_state *ss, action *a, player *pl,
						 server *s) {
  event e;
  int tmp;
  card_color col;

  if (ss->sgs.alleinspieler == -1 || ss->sgs.alleinspieler != pl->ap) {
	DEBUG_PRINTF("Invalid spielansagen actor");
	return GAME_PHASE_INVALID;
  }

  e.answer_to = a->id;
  e.acting_player = pl->gupid;

  switch (a->type) {
	case ACTION_CALL_GAME:
	  if (ss->sgs.took_skat && a->gr.hand) {
		DEBUG_PRINTF("WOW, stupid idiot, you can't play hand if you already "
					 "took the skat. Bonk.");
		return GAME_PHASE_INVALID;
	  }

	  if (a->gr.type == GAME_TYPE_INVALID) {
		DERROR_PRINTF("Received GAME_PHASE_INVALID");
		return GAME_PHASE_INVALID;
	  }

	  switch (a->gr.type) {
		case GAME_TYPE_GRAND:
		  if (a->gr.trumpf != COLOR_INVALID)
			return GAME_PHASE_INVALID;
		  goto skip_color_check;
		case GAME_TYPE_COLOR:
		  col = a->gr.trumpf;
		  if (col != COLOR_KREUZ && col != COLOR_PIK && col != COLOR_HERZ
			  && col != COLOR_KARO)
			return GAME_PHASE_INVALID;

		skip_color_check:
		  tmp = a->gr.hand | (a->gr.schneider_angesagt << 1)
				| (a->gr.schwarz_angesagt << 2) | (a->gr.ouvert << 3);

		  if (tmp & (tmp + 1))
			return GAME_PHASE_INVALID;
		  break;
		case GAME_TYPE_NULL:
		  if (a->gr.trumpf != COLOR_INVALID)
			return GAME_PHASE_INVALID;
		  if (a->gr.schwarz_angesagt || a->gr.schneider_angesagt)
			return GAME_PHASE_INVALID;
		  break;
		  // TODO: GAME_TYPE_RAMSCH
		default:
		  return GAME_PHASE_INVALID;
	  }

	  ss->sgs.gr = a->gr;

	  e.type = EVENT_GAME_CALLED;
	  e.gr = a->gr;
	  server_distribute_event(s, &e, NULL);

	  return GAME_PHASE_PLAY_STICH_C1;
	default:
	  DEBUG_PRINTF("Trying to use undefined action %s in state %s",
				   action_name_table[a->type],
				   game_phase_name_table[ss->sgs.cgphase]);
	  return GAME_PHASE_INVALID;
  }
}

static int
next_active_player(int player, int off) {
  return (player + off) % 3;
}

static game_phase
apply_action_stich(skat_server_state *ss, action *a, player *pl, server *s,
				   int ind) {
  event e;
  player *expected_player;
  int expected_player_gupid;
  int curr, result;
  int winnerv;// indexed by vorhand + ap
  int winner; // indexed by ap

  switch (a->type) {
	case ACTION_PLAY_CARD:
	  curr = next_active_player(ss->sgs.curr_stich.vorhand, ind);
	  expected_player_gupid = ss->sgs.active_players[curr];
	  expected_player = s->pls[expected_player_gupid];
	  if (pl->gupid != expected_player_gupid) {
		DEBUG_PRINTF("Wrong player trying to play card: Expected player %s "
					 "(gupid: %d), but got %s (gupid %d) instead",
					 expected_player->name, expected_player_gupid, pl->name,
					 pl->gupid);

		return GAME_PHASE_INVALID;
	  }
	  if (stich_card_legal(&ss->sgs.gr, &ss->sgs.curr_stich, &a->card,
						   &ss->player_hands[curr], &result)
		  || !result) {
		char buf[4];
		card_get_name(&a->card, buf);
		DEBUG_PRINTF("Trying to play illegal card %s", buf);
		return GAME_PHASE_INVALID;
	  }

	  e.type = EVENT_PLAY_CARD;
	  e.answer_to = a->id;
	  e.acting_player = pl->gupid;
	  e.card = a->card;
	  server_distribute_event(s, &e, NULL);

	  card_collection_remove_card(&ss->player_hands[curr], &a->card);

	  if (!ind) {
		ss->sgs.curr_stich.cs[0] = a->card;
		ss->sgs.curr_stich.played_cards = 1;
		return GAME_PHASE_PLAY_STICH_C2;
	  } else if (ind == 1) {
		ss->sgs.curr_stich.cs[1] = a->card;
		ss->sgs.curr_stich.played_cards = 2;
		return GAME_PHASE_PLAY_STICH_C3;
	  } else {// ind == 2
		ss->sgs.curr_stich.cs[2] = a->card;
		ss->sgs.curr_stich.played_cards = 3;
	  }

	  stich_get_winner(&ss->sgs.gr, &ss->sgs.curr_stich,
					   &winnerv);// Sue me for discarding the return value

	  winner = next_active_player(ss->sgs.curr_stich.vorhand, winnerv);
	  ss->sgs.curr_stich.winner = winner;

	  card_collection_add_card_array(ss->stiche[winner], ss->sgs.curr_stich.cs,
									 3);

	  ss->sgs.stich_num++;

	  e.type = EVENT_STICH_DONE;
	  e.answer_to = -1;
	  e.acting_player = -1;
	  e.stich_winner = ss->sgs.active_players[winner];
	  server_distribute_event(s, &e, NULL);

	  ss->sgs.last_stich = ss->sgs.curr_stich;
	  ss->sgs.curr_stich =
			  (stich){.vorhand = ss->sgs.last_stich.winner, .winner = -1};

	  // cancel game instantly in null spiel if alleinspieler wins a stich
	  if (ss->sgs.gr.type == GAME_TYPE_NULL
		  && ss->sgs.alleinspieler == winner) {
		// TODO: for general game cancel (giving up, showing cards etc.) give
		// all remaining cards to alleinspieler?
		goto round_done;
	  } else if (ss->sgs.stich_num <= 9) {
		return GAME_PHASE_PLAY_STICH_C1;
	  }

	round_done:
	  skat_calculate_game_result(ss, &e.rr);

	  e.answer_to = -1;
	  e.type = EVENT_ANNOUNCE_SCORES;
	  server_distribute_event(s, &e, NULL);

	  for (int i = 0; i < 3; i++)
		ss->sgs.score[ss->sgs.active_players[i]] += e.rr.round_score[i];

	  memcpy(e.score_total, ss->sgs.score, sizeof ss->sgs.score);
	  e.answer_to = -1;
	  e.type = EVENT_ROUND_DONE;
	  server_distribute_event(s, &e, NULL);

	  return GAME_PHASE_BETWEEN_ROUNDS;
	default:
	  DEBUG_PRINTF("Trying to use undefined action %s in state %s",
				   action_name_table[a->type],
				   game_phase_name_table[ss->sgs.cgphase]);
	  return GAME_PHASE_INVALID;
  }
}

static game_phase
apply_action(skat_server_state *ss, action *a, player *pl, server *s) {
  DEBUG_PRINTF("Applying action %s in skat state %s",
			   action_name_table[a->type],
			   game_phase_name_table[ss->sgs.cgphase]);
  switch (ss->sgs.cgphase) {
	case GAME_PHASE_SETUP:
	  return apply_action_setup(ss, a, pl, s);
	case GAME_PHASE_BETWEEN_ROUNDS:
	  return apply_action_between_rounds(ss, a, pl, s);
	case GAME_PHASE_REIZEN:
	  return apply_action_reizen(ss, a, pl, s);
	case GAME_PHASE_SKAT_AUFNEHMEN:
	  return apply_action_skat_aufnehmen(ss, a, pl, s);
	case GAME_PHASE_SPIELANSAGE:
	  return apply_action_spielansage(ss, a, pl, s);
	case GAME_PHASE_PLAY_STICH_C1:
	  return apply_action_stich(ss, a, pl, s, 0);
	case GAME_PHASE_PLAY_STICH_C2:
	  return apply_action_stich(ss, a, pl, s, 1);
	case GAME_PHASE_PLAY_STICH_C3:
	  return apply_action_stich(ss, a, pl, s, 2);
	default:
	  DERROR_PRINTF("Undefined Gamestate encountered!");
	  return GAME_PHASE_INVALID;
  }
}

int
skat_server_state_apply(skat_server_state *ss, action *a, player *pl,
						server *s) {
  DEBUG_PRINTF("Applying action %s by player '%s'", action_name_table[a->type],
			   pl->name);

  game_phase new;
  new = apply_action(ss, a, pl, s);
  if (new == GAME_PHASE_INVALID)
	return 0;
  ss->sgs.cgphase = new;
  return 1;
}

void
skat_server_state_tick(skat_server_state *ss, server *s) {}

static int
skat_client_handle_reizen_events(skat_client_state *cs, event *e, client *c) {
  if (cs->sgs.cgphase != GAME_PHASE_REIZEN
	  && cs->sgs.cgphase != GAME_PHASE_CLIENT_WAIT_REIZEN_DONE) {
	DERROR_PRINTF("Trying to apply event %s, while in invalid game state %s",
				  event_name_table[e->type],
				  game_phase_name_table[cs->sgs.cgphase]);
	return 0;
  }

  reiz_state *rs = &cs->sgs.rs;

  if (rs->rphase == REIZ_PHASE_INVALID || rs->rphase == REIZ_PHASE_DONE) {
	DERROR_PRINTF("Trying to apply event %s, while in invalid reiz phase %s",
				  event_name_table[e->type], reiz_phase_name_table[rs->rphase]);
	return 0;
  }

  switch (e->type) {
	case EVENT_REIZEN_NUMBER:
	  rs->reizwert = e->reizwert;
	  rs->waiting_teller = 0;
	  if (rs->rphase == REIZ_PHASE_WINNER)
		cs->sgs.cgphase = GAME_PHASE_CLIENT_WAIT_REIZEN_DONE;
	  return 1;
	case EVENT_REIZEN_CONFIRM:
	  rs->waiting_teller = 1;
	  if (rs->rphase == REIZ_PHASE_WINNER) {
		if (rs->reizwert < 18)
		  rs->reizwert = 18;
		cs->sgs.cgphase = GAME_PHASE_CLIENT_WAIT_REIZEN_DONE;
	  }
	  return 1;
	case EVENT_REIZEN_PASSE:
	  if (rs->rphase == REIZ_PHASE_MITTELHAND_TO_VORHAND) {
		rs->rphase = REIZ_PHASE_HINTERHAND_TO_WINNER;
		rs->winner = !rs->waiting_teller;
		rs->waiting_teller = 1;
	  } else if (rs->rphase == REIZ_PHASE_HINTERHAND_TO_WINNER) {
		if (!rs->waiting_teller)
		  rs->winner = 2;
		rs->waiting_teller = 1;

		if (rs->reizwert >= 18) {
		  cs->sgs.cgphase = GAME_PHASE_CLIENT_WAIT_REIZEN_DONE;
		} else {
		  rs->rphase = REIZ_PHASE_WINNER;
		}
	  } else {
		cs->sgs.cgphase = GAME_PHASE_CLIENT_WAIT_REIZEN_DONE;
	  }
	  return 1;
	case EVENT_REIZEN_DONE:
	  if (cs->sgs.cgphase != GAME_PHASE_CLIENT_WAIT_REIZEN_DONE) {
		DERROR_PRINTF(
				"Trying to apply event %s, while in invalid game state %s",
				event_name_table[e->type],
				game_phase_name_table[cs->sgs.cgphase]);
		return 0;
	  }

	  rs->rphase = REIZ_PHASE_DONE;
	  rs->waiting_teller = 1;
	  rs->reizwert = e->reizwert_final;
	  cs->sgs.alleinspieler = rs->winner = e->alleinspieler;

	  if (rs->reizwert < 18) {
		cs->sgs.gr.type = GAME_TYPE_RAMSCH;
		// TODO: implement schieberamsch
		cs->sgs.cgphase = GAME_PHASE_PLAY_STICH_C1;
	  } else {
		cs->sgs.cgphase = GAME_PHASE_SKAT_AUFNEHMEN;
	  }

	  if (cs->my_active_player_index == cs->sgs.alleinspieler
		  || cs->sgs.gr.type == GAME_TYPE_RAMSCH) {
		cs->ist_alleinspieler = 1;
		cs->my_partner = -1;
	  } else {
		cs->ist_alleinspieler = 0;

		// 0,1 | 1,0, sum=1 -> 2
		// 0,2 | 2,0, sum=2 -> 1
		// 1,2 | 2,1, sum=3 -> 3

		int sum = cs->my_active_player_index + cs->sgs.alleinspieler;
		if (sum == 1)
		  cs->my_partner = 2;
		else if (sum == 2)
		  cs->my_partner = 1;
		else
		  cs->my_partner = 3;
	  }

	  return 1;
	default:
	  DERROR_PRINTF("Trying to apply event %s, while handling reizen events, "
					"this should not happen",
					event_name_table[e->type]);
	  return 0;
  }
}

static int
skat_client_handle_skat_events(skat_client_state *cs, event *e, client *c) {
  if (cs->sgs.cgphase != GAME_PHASE_SKAT_AUFNEHMEN) {
	DERROR_PRINTF("Trying to apply event %s, while in invalid game state %s",
				  event_name_table[e->type],
				  game_phase_name_table[cs->sgs.cgphase]);
	return 0;
  }

  switch (e->type) {
	case EVENT_SKAT_TAKE:
	  cs->sgs.took_skat = 1;
	  if (cs->ist_alleinspieler)
		card_collection_add_card_array(&cs->my_hand, e->skat, 2);
	  return 1;
	case EVENT_SKAT_LEAVE:
	  cs->sgs.took_skat = 0;
	  cs->sgs.cgphase = GAME_PHASE_SPIELANSAGE;
	  return 1;
	case EVENT_SKAT_PRESS:
	  if (cs->ist_alleinspieler)
		card_collection_remove_card_array(&cs->my_hand, e->skat_press_cards, 2);
	  cs->sgs.cgphase = GAME_PHASE_SPIELANSAGE;
	  return 1;
	default:
	  DERROR_PRINTF("Trying to apply event %s, while handling skat events, "
					"this should not happen",
					event_name_table[e->type]);
	  return 0;
  }
}

int
skat_client_state_apply(skat_client_state *cs, event *e, client *c) {
  DTODO_PRINTF("Insert sanity checks.");
  char card_name_buf[4];
  switch (e->type) {
	case EVENT_START_GAME:
	  DEBUG_PRINTF("Starting game");

	  cs->sgs.cgphase = GAME_PHASE_BETWEEN_ROUNDS;
	  return 1;
	case EVENT_START_ROUND:
	  DEBUG_PRINTF("Starting round");

	  memcpy(cs->sgs.active_players, e->current_active_players,
			 sizeof(cs->sgs.active_players));

	  for (int gupid = 0; gupid < 4; gupid++) {
		if (c->pls[gupid])
		  c->pls[gupid]->ap = -1;
	  }

	  for (int ap = 0; ap < 3; ap++) {
		c->pls[c->cs.sgs.active_players[ap]]->ap = ap;
	  }

	  cs->my_active_player_index = c->pls[c->cs.my_gupid]->ap;

	  return 1;
	case EVENT_DISTRIBUTE_CARDS:
	  DEBUG_PRINTF("Distributing cards");

	  cs->my_hand = e->hand;

	  cs->sgs.curr_stich =
			  (stich){.played_cards = 0, .vorhand = 0, .winner = -1};
	  cs->sgs.last_stich =
			  (stich){.played_cards = 0, .vorhand = -1, .winner = -1};
	  cs->sgs.stich_num = 0;
	  cs->sgs.alleinspieler = -1;

	  cs->sgs.rs.rphase = REIZ_PHASE_MITTELHAND_TO_VORHAND;
	  cs->sgs.rs.waiting_teller = 1;
	  cs->sgs.rs.reizwert = 0;
	  cs->sgs.rs.winner = -1;

	  cs->my_partner = -1;
	  cs->ist_alleinspieler = -1;

	  cs->sgs.cgphase = GAME_PHASE_REIZEN;
	  return 1;
	case EVENT_REIZEN_NUMBER:
	case EVENT_REIZEN_CONFIRM:
	case EVENT_REIZEN_PASSE:
	case EVENT_REIZEN_DONE:
	  return skat_client_handle_reizen_events(cs, e, c);
	case EVENT_SKAT_TAKE:
	case EVENT_SKAT_LEAVE:
	case EVENT_SKAT_PRESS:
	  return skat_client_handle_skat_events(cs, e, c);
	case EVENT_GAME_CALLED:
	  cs->sgs.gr = e->gr;
	  cs->sgs.cgphase = GAME_PHASE_PLAY_STICH_C1;
	  return 1;
	case EVENT_PLAY_CARD:
	  card_get_name(&e->card, card_name_buf);
	  DEBUG_PRINTF("%s (%d) played card %s", c->pls[e->acting_player]->name,
				   e->acting_player, card_name_buf);
	  if (c->cs.my_gupid == e->acting_player) {
		card_collection_remove_card(&cs->my_hand, &e->card);
	  }

	  if (cs->sgs.cgphase == GAME_PHASE_PLAY_STICH_C1) {
		cs->sgs.curr_stich.cs[0] = e->card;
		cs->sgs.curr_stich.played_cards = 1;
		for (int i = 0; i < 3; ++i) {
		  if (cs->sgs.active_players[i] == e->acting_player) {
			cs->sgs.curr_stich.vorhand = i;
			break;
		  }
		}
		cs->sgs.cgphase = GAME_PHASE_PLAY_STICH_C2;
	  } else if (cs->sgs.cgphase == GAME_PHASE_PLAY_STICH_C2) {
		cs->sgs.curr_stich.cs[1] = e->card;
		cs->sgs.curr_stich.played_cards = 2;
		cs->sgs.cgphase = GAME_PHASE_PLAY_STICH_C3;
	  } else if (cs->sgs.cgphase == GAME_PHASE_PLAY_STICH_C3) {
		cs->sgs.curr_stich.cs[2] = e->card;
		cs->sgs.curr_stich.played_cards = 3;
		cs->sgs.cgphase = GAME_PHASE_CLIENT_WAIT_STICH_DONE;
	  } else {
		DERROR_PRINTF("Invalid game phase %s for PLAY_CARD",
					  game_phase_name_table[cs->sgs.cgphase]);
		return 0;
	  }
	  return 1;
	case EVENT_STICH_DONE:
	  DEBUG_PRINTF("Stich done");

	  if (cs->sgs.cgphase != GAME_PHASE_CLIENT_WAIT_STICH_DONE) {
		DERROR_PRINTF("Invalid game phase %s for STICH_DONE",
					  game_phase_name_table[cs->sgs.cgphase]);
		return 0;
	  }

	  cs->sgs.curr_stich.winner = c->pls[e->stich_winner]->ap;

	  if (++cs->sgs.stich_num <= 9)
		cs->sgs.cgphase = GAME_PHASE_PLAY_STICH_C1;
	  else
		cs->sgs.cgphase = GAME_PHASE_CLIENT_WAIT_ANNOUNCE_SCORES;

	  cs->sgs.last_stich = cs->sgs.curr_stich;
	  cs->sgs.curr_stich =
			  (stich){.vorhand = cs->sgs.last_stich.winner, .winner = -1};

	  return 1;
	case EVENT_ANNOUNCE_SCORES:
	  DEBUG_PRINTF("Scores are being announced, everyone is shivering with "
				   "excitement");

	  // Allow round done in all phases for game cancel
	  /*if (cs->sgs.cgphase != GAME_PHASE_CLIENT_WAIT_ANNOUNCE_SCORES) {
		DERROR_PRINTF("Invalid game phase %s for ANNOUNCE_SCORES",
					  game_phase_name_table[cs->sgs.cgphase]);
		return 0;
	  }*/

	  cs->sgs.cgphase = GAME_PHASE_CLIENT_WAIT_ROUND_DONE;

	  return 1;
	case EVENT_ROUND_DONE:
	  DEBUG_PRINTF("Round done");

	  if (cs->sgs.cgphase != GAME_PHASE_CLIENT_WAIT_ROUND_DONE) {
		DERROR_PRINTF("Invalid game phase %s for ROUND_DONE",
					  game_phase_name_table[cs->sgs.cgphase]);
		return 0;
	  }

	  memcpy(cs->sgs.score, e->score_total, sizeof cs->sgs.score);

	  cs->sgs.cgphase = GAME_PHASE_BETWEEN_ROUNDS;

	  return 1;
	default:
	  DERROR_PRINTF(
			  "Trying to apply event %s, but it isn't implemented or illegal",
			  event_name_table[e->type]);
	  return 0;
  }
}

void
skat_client_state_tick(skat_client_state *cs, client *c) {}

void
skat_resync_player(skat_server_state *ss, skat_client_state *cs, player *pl) {
  memset(cs, '\0', sizeof(skat_client_state));

  cs->sgs = ss->sgs;

  get_player_hand(ss, pl, &cs->my_hand);

  cs->my_gupid = pl->gupid;
  cs->my_active_player_index = pl->ap;

  if (cs->my_active_player_index == -1) {
	cs->ist_alleinspieler = -1;
	cs->my_partner = -1;
  } else if (cs->my_active_player_index == cs->sgs.alleinspieler
			 || cs->sgs.gr.type == GAME_TYPE_RAMSCH) {
	cs->ist_alleinspieler = 1;
	cs->my_partner = cs->my_active_player_index;
  } else {
	cs->ist_alleinspieler = 0;

	// 0,1 | 1,0, sum=1 -> 2
	// 0,2 | 2,0, sum=2 -> 1
	// 1,2 | 2,1, sum=3 -> 3

	int sum = cs->my_active_player_index + cs->sgs.alleinspieler;
	if (sum == 1)
	  cs->my_partner = 2;
	else if (sum == 2)
	  cs->my_partner = 1;
	else
	  cs->my_partner = 3;
  }
}

void
server_skat_state_init(skat_server_state *ss) {
  ss->sgs.cgphase = GAME_PHASE_SETUP;
  memset(ss->sgs.score, 0, sizeof(ss->sgs.score));
  memset(ss->sgs.active_players, -1, sizeof(ss->sgs.active_players));
}

void
client_skat_state_init(skat_client_state *cs) {
  cs->sgs.cgphase = GAME_PHASE_SETUP;
  memset(cs->sgs.score, '\0', sizeof(cs->sgs.score));
  memset(cs->sgs.active_players, -1, sizeof(cs->sgs.active_players));

  cs->my_partner = cs->my_active_player_index = -1;
}

void
client_skat_state_notify_join(skat_client_state *cs,
							  payload_notify_join *pl_nj) {}

void
client_skat_state_notify_leave(skat_client_state *cs,
							   payload_notify_leave *pl_nl) {}
