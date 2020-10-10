#include "skat/skat.h"
#include "skat/server.h"
#include "skat/util.h"
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

void
skat_calculate_game_result(skat_server_state *ss, int *score) {
  DTODO_PRINTF("TODO: implement calculate_game_result");// TODO: implement
}

static void
get_player_hand(skat_server_state *ss, player *pl, card_collection *col) {
  for (int i = 0; i < 3; ++i) {
	if (ss->sgs.active_players[i] == pl->index) {
	  *col = ss->player_hands[i];
	  return;
	}
  }

  card_collection_empty(col);
}

// Conforming to the rules. Poggers.
static int
distribute_cards(skat_server_state *ss) {
  card_collection draw_pile;
  card_collection_fill(&draw_pile);

  card_id cid;
  for (int i = 0; i < 3; i++) {
	for (int j = 0; j < 3; j++) {
	  card_collection_draw_random(&draw_pile, &cid);
	  card_collection_add_card(&ss->player_hands[i], &cid);
	  card_collection_remove_card(&draw_pile, &cid);
	}
  }

  for (int i = 0; i < 2; i++) {
	card_collection_draw_random(&draw_pile, &cid);
	ss->skat[i] = cid;
	card_collection_remove_card(&draw_pile, &cid);
  }

  for (int i = 0; i < 3; i++) {
	for (int j = 0; j < 4; j++) {
	  card_collection_draw_random(&draw_pile, &cid);
	  card_collection_add_card(&ss->player_hands[i], &cid);
	  card_collection_remove_card(&draw_pile, &cid);
	}
  }

  for (int i = 0; i < 3; i++) {
	for (int j = 0; j < 3; j++) {
	  card_collection_draw_random(&draw_pile, &cid);
	  card_collection_add_card(&ss->player_hands[i], &cid);
	  card_collection_remove_card(&draw_pile, &cid);
	}
  }

  return 0;
}

static game_phase
apply_action_setup(skat_server_state *ss, action *a, player *pl, server *s) {
  event e;
  e.answer_to = a->id;
  e.player = pl->name;
  switch (a->type) {
	case ACTION_READY:
	  if (s->ncons < 3) {
	    DEBUG_PRINTF("Rejecting action ACTION_READY with id %ld by player %s because "
			 		 "s->ncons = %d < 3", a->id, pl->name.name, s->ncons);
		return GAME_PHASE_INVALID;
	  }

	  e.type = EVENT_START_GAME;
	  server_distribute_event(s, &e, NULL);

	  return GAME_PHASE_BETWEEN_ROUNDS;
	case ACTION_RULE_CHANGE:
	default:
	  return GAME_PHASE_INVALID;
  }
}

static game_phase
apply_action_between_rounds(skat_server_state *ss, action *a, player *pl,
							server *s) {
  int pm, ix;
  event e;
  e.answer_to = a->id;
  e.player = pl->name;
  switch (a->type) {
	case ACTION_READY:
	  if (s->ncons < 3) {

	    DEBUG_PRINTF("Rejecting action ACTION_READY with id %ld by player %s because "
			 		 "s->ncons = %d < 3", a->id, pl->name.name, s->ncons);

		return GAME_PHASE_INVALID;
	  }

	  e.answer_to = -1;
	  e.type = EVENT_START_ROUND;

	  if (ss->sgs.active_players[0] == -1) {
		for (int i = 0, j = 0; i < 4; i++)
		  if ((s->playermask >> i) & 1)
			ss->sgs.active_players[j++] = s->ps[i]->index;
	  } else if (s->ncons == 3) {// we don't have a spectator
		perm(ss->sgs.active_players, 3, 0x12);
	  } else {
		pm = 0;
		for (int i = 0; i < 3; i++)
		  pm |= 1 << ss->sgs.active_players[i];
		ix = __builtin_ctz(~pm);
		perm(ss->sgs.active_players, 3, 0x12);
		ss->sgs.active_players[2] = s->ps[ix]->index;
	  }

	  memcpy(e.current_active_players, ss->sgs.active_players, 3 * sizeof(int));

	  server_distribute_event(s, &e, NULL);

	  ss->sgs.curr_stich = (stich){.vorhand = 0, .winner = -1};
	  ss->sgs.last_stich = (stich){.vorhand = -1, .winner = -1};
	  ss->sgs.stich_num = 0;
	  ss->sgs.alleinspieler = -1;
	  ss->spielwert = -1;
	  memset(ss->stiche, '\0', 3 * sizeof(ss->stiche[0]));
	  card_collection_empty(&ss->stiche_buf[0]);
	  card_collection_empty(&ss->stiche_buf[1]);
	  card_collection_empty(&ss->stiche_buf[2]);

	  distribute_cards(ss);

      DEBUG_PRINTF("Player hands: %#x, %#x, %#x", ss->player_hands[0],
			 	   ss->player_hands[1], ss->player_hands[2]);

	  e.type = EVENT_DISTRIBUTE_CARDS;

	  void mask_hands(event * ev, player * pl) {
		get_player_hand(ss, pl, &ev->hand);
	  }

	  server_distribute_event(s, &e, mask_hands);

	  DTODO_PRINTF("TODO: implement reizen");// TODO: implement reizen
	  // return GAME_PHASE_REIZEN_BEGIN;
	  ss->spielwert = 18;
	  ss->stiche[0] = &ss->stiche_buf[0];
	  ss->stiche[1] = &ss->stiche_buf[1];
	  ss->stiche[2] = &ss->stiche_buf[1];
	  card_collection_add_card_array(ss->stiche[0], ss->skat, 2);
	  ss->sgs.alleinspieler = 0;
	  ss->sgs.gr = (game_rules){.type = GAME_TYPE_COLOR, .trumpf = COLOR_KREUZ};
	  ss->sgs.rr = (reiz_resultat){.reizwert = 18,
								   .hand = 0,
								   .schneider = 0,
								   .schwarz = 0,
								   .ouvert = 0,
								   .contra = 0,
								   .re = 0};
	  return GAME_PHASE_PLAY_STICH_C1;
	default:
	  return GAME_PHASE_INVALID;
  }
}

static game_phase
apply_action_reizen_begin(skat_server_state *ss, action *a, player *pl,
						  server *s) {
  // remember to initialize stiche!
  event e;
  e.answer_to = a->id;
  e.player = pl->name;
  DTODO_PRINTF("TODO: implement reizen");// TODO: implement reizen
  switch (a->type) {
	default:
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
  int curr, result;
  int winnerv;// indexed by vorhand + ap
  int winner; // indexed by ap

  switch (a->type) {
	case ACTION_PLAY_CARD:
	  curr = next_active_player(ss->sgs.curr_stich.vorhand, ind);
	  if (!player_equals_by_name(pl, server_get_player_by_gupid(
											 s, ss->sgs.active_players[curr])))
		return GAME_PHASE_INVALID;
	  if (stich_card_legal(&ss->sgs.gr, ss->sgs.curr_stich.cs, ind, &a->card,
						   &ss->player_hands[curr], &result)
		  || !result)
		return GAME_PHASE_INVALID;

	  e.type = EVENT_PLAY_CARD;
	  e.answer_to = a->id;
	  e.player = pl->name;
	  e.card = a->card;
	  server_distribute_event(s, &e, NULL);

	  if (!ind) {
		ss->sgs.curr_stich.cs[0] = a->card;
		return GAME_PHASE_PLAY_STICH_C2;
	  } else if (ind == 1) {
		ss->sgs.curr_stich.cs[1] = a->card;
		return GAME_PHASE_PLAY_STICH_C3;
	  }

	  stich_get_winner(&ss->sgs.gr, &ss->sgs.curr_stich, &winnerv);// Sue me

	  winner = next_active_player(ss->sgs.curr_stich.vorhand, winnerv);
	  ss->sgs.curr_stich.winner = winner;

	  card_collection_add_card_array(ss->stiche[winner], ss->sgs.curr_stich.cs,
									 3);

	  e.type = EVENT_STICH_DONE;
	  e.answer_to = -1;
	  e.stich_winner = s->ps[ss->sgs.active_players[winner]]->name;
	  server_distribute_event(s, &e, NULL);

	  ss->sgs.last_stich = ss->sgs.curr_stich;
	  ss->sgs.curr_stich =
			  (stich){.vorhand = ss->sgs.last_stich.winner, .winner = -1};

	  if (ss->sgs.stich_num++ < 9)
		return GAME_PHASE_PLAY_STICH_C1;

	  skat_calculate_game_result(ss, e.score_round);

	  for (int i = 0; i < 3; i++)
		ss->sgs.score[ss->sgs.active_players[i]] += e.score_round[i];

	  e.answer_to = -1;
	  e.type = EVENT_ROUND_DONE;
	  server_distribute_event(s, &e, NULL);

	  return GAME_PHASE_BETWEEN_ROUNDS;
	default:
	  return GAME_PHASE_INVALID;
  }
}
/*
static game_phase
apply_action_stich(skat_state *ss, action *a, player *pl, server *s, int card) {
  event e;
  e.answer_to = a->id;
  e.player = pl->id;
  int cpi, result;
  switch (a->type) {
	case ACTION_PLAY_CARD:
	  cpi = (ss->sgs.curr_stich.vorhand + card) % 3;
	  if (!player_name_equals(&pl->id, &ss->sgs.active_players[cpi]))
		return GAME_PHASE_INVALID;
	  if (stich_card_legal(&ss->sgs.gr, ss->sgs.curr_stich.cs, &card, &a->card,
						   &ss->player_hands[cpi], &result)
		  || !result)
		return GAME_PHASE_INVALID;
	  ss->sgs.curr_stich.cs[card] = a->card;

	  e.type = EVENT_PLAY_CARD;
	  e.card = a->card;

	  server_distribute_event(s, &e, NULL);

	  if (!card)
		return GAME_PHASE_PLAY_STICH_C2;
	  if (card == 1)
		return GAME_PHASE_PLAY_STICH_C3;

	  stich_get_winner(&ss->sgs.gr, &ss->sgs.curr_stich,
					   &ss->sgs.curr_stich.winner);

	  card_collection_add_card_array(ss->stiche[ss->sgs.curr_stich.winner],
									 ss->sgs.curr_stich.cs, 3);

	  e.answer_to = -1;
	  e.type = EVENT_STICH_DONE;
	  e.stich_winner = ss->sgs.active_players[ss->sgs.curr_stich.winner];
	  server_distribute_event(s, &e, NULL);

	  ss->sgs.last_stich = ss->sgs.curr_stich;
	  ss->sgs.curr_stich =
			  (stich){.vorhand = ss->sgs.last_stich.winner, .winner = -1};

	  if (ss->sgs.stich_num++ < 9)
		return GAME_PHASE_PLAY_STICH_C1;

	  // game finished
	  skat_calculate_game_result(ss, e.score_round);

	  for (int i = 0; i < 3; i++)
		ss->sgs.total_score[(ss->sgs.last_active_player_index + i)
							% s->ncons] += e.score_round[i];

	  e.answer_to = -1;
	  e.type = EVENT_ROUND_DONE;
	  server_distribute_event(s, &e, NULL);

	  return GAME_PHASE_BETWEEN_ROUNDS;
	default:
	  return GAME_PHASE_INVALID;
  }
}
*/

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
	case GAME_PHASE_REIZEN_BEGIN:
	  return apply_action_reizen_begin(ss, a, pl, s);
	case GAME_PHASE_PLAY_STICH_C1:
	  return apply_action_stich(ss, a, pl, s, 0);
	case GAME_PHASE_PLAY_STICH_C2:
	  return apply_action_stich(ss, a, pl, s, 1);
	case GAME_PHASE_PLAY_STICH_C3:
	  return apply_action_stich(ss, a, pl, s, 2);
	default:
	  return GAME_PHASE_INVALID;
  }
}

int
skat_server_state_apply(skat_server_state *ss, action *a, player *pl,
						server *s) {
  DEBUG_PRINTF("Applying action %s by player '%s'", action_name_table[a->type],
			   pl->name.name);

  game_phase new;
  new = apply_action(ss, a, pl, s);
  if (new == GAME_PHASE_INVALID)
	return 0;
  ss->sgs.cgphase = new;
  return 1;
}

void
skat_server_state_tick(skat_server_state *ss, server *s) {}

int
skat_client_state_apply(skat_client_state *cs, event *e, client *c) {
  DTODO_PRINTF("Insert sanity checks.");
  switch(e->type) {
	case EVENT_START_GAME:
	  cs->sgs.cgphase = GAME_PHASE_BETWEEN_ROUNDS;
	  return 1;	
	case EVENT_START_ROUND:
	  cs->sgs.cgphase = GAME_PHASE_PLAY_STICH_C1;
	  return 1;	
	case EVENT_DISTRIBUTE_CARDS:
	  cs->my_hand = e->hand;
	  return 1;
	default:
      DERROR_PRINTF("Trying to apply event %s, but it isn't implement or illegal", event_name_table[e->type]);
	  return 0;
  }
  return 0;
  /*
  game_phase new;
  new = apply_action(ss, a, pl, s);
  if (new == GAME_PHASE_INVALID)
	return 0;
  ss->sgs.cgphase = new;
  return 1;
   */
}

void
skat_client_state_tick(skat_client_state *cs, client *c) {}

void
skat_resync_player(skat_server_state *ss, skat_client_state *cs, player *pl) {
  memset(cs, '\0', sizeof(skat_client_state));

  cs->sgs = ss->sgs;
  cs->my_index = pl->index;
  get_player_hand(ss, pl, &cs->my_hand);
  if (ss->sgs.cgphase == GAME_PHASE_SKAT_AUFNEHMEN
	  && pl->index == ss->sgs.alleinspieler) {
	cs->skat[0] = ss->skat[0];
	cs->skat[1] = ss->skat[1];
  }
}

void
skat_state_init(skat_server_state *ss) {
  ss->sgs.cgphase = GAME_PHASE_SETUP;
  memset(ss->sgs.score, 0, sizeof(ss->sgs.score));
  memset(ss->sgs.active_players, -1, 3 * sizeof(int));
}

void
client_skat_state_notify_join(skat_client_state *cs,
							  payload_notify_join *pl_nj) {
  DTODO_PRINTF("We don't yet know what to do here");// TODO: this
}

void
client_skat_state_notify_leave(skat_client_state *cs,
							   payload_notify_leave *pl_nl) {
  DTODO_PRINTF("... Neither do we here");// TODO: this
}
