#include "skat/skat.h"
#include "skat/server.h"

int
game_setup_server(skat_state *ss) {
  ss->sgs.cgphase = GAME_PHASE_SETUP;
  return 0;
}

int
game_start_server(skat_state *ss) {
  // TODO: implement
  return 0;
}

void
skat_state_notify_disconnect(skat_state *ss, player *pl, server *s) {
  // TODO: implement
}
void
skat_state_notify_join(skat_state *ss, player *pl, server *s) {
  // TODO: implement
}

// returns pos+1 on find, 0 otherwise
static int
is_active_player(shared_game_state *sgs, player *pl) {
  return -1;
}

static card_collection
get_player_hand(skat_state *ss, player *pl) {
  return -1;
}

// Conforming to the rules. Poggers.
static int
distribute_cards(skat_state *ss) {
  card_collection draw_pile;
  card_collection_fill(&draw_pile);

  card_id cid;
  for (int i = 0; i < 3; i++) {
	for (int j = 0; j < 3; j++) {
	  card_collection_draw_random(&draw_pile, &cid);
	  card_collection_add_card(&ss->player_hands[i], &cid);
	}
  }

  for (int i = 0; i < 2; i++) {
	card_collection_draw_random(&draw_pile, &cid);
	ss->skat[i] = cid;
  }

  for (int i = 0; i < 3; i++) {
	for (int j = 0; j < 4; j++) {
	  card_collection_draw_random(&draw_pile, &cid);
	  card_collection_add_card(&ss->player_hands[i], &cid);
	}
  }

  for (int i = 0; i < 3; i++) {
	for (int j = 0; j < 3; j++) {
	  card_collection_draw_random(&draw_pile, &cid);
	  card_collection_add_card(&ss->player_hands[i], &cid);
	}
  }

  return 0;
}

static game_phase
apply_action_setup(skat_state *ss, action *a, player *pl, server *s) {
  event e;
  e.answer_to = a->id;
  e.player = pl->id;
  switch (a->type) {
	case ACTION_READY:
	  if (ss->sgs.num_players < 3)
		return GAME_PHASE_INVALID;

	  e.type = EVENT_START_GAME;
	  server_distribute_event(s, &e, NULL);

	  ss->sgs.last_active_player_index = 0;

	  return GAME_PHASE_BETWEEN_ROUNDS;
	case ACTION_RULE_CHANGE:
	default:
	  return GAME_PHASE_INVALID;
  }
}

static game_phase
apply_action_between_rounds(skat_state *ss, action *a, player *pl, server *s) {
  event e;
  e.answer_to = a->id;
  e.player = pl->id;
  switch (a->type) {
	case ACTION_READY:
	  if (ss->sgs.num_players < 3)
		return GAME_PHASE_INVALID;

	  e.type = EVENT_START_ROUND;
	  server_distribute_event(s, &e, NULL);

	  e.answer_to = -1;
	  e.type = EVENT_START_ROUND;

	  e.current_active_players[0] = s->ps[ss->last_active_player_index].id;
	  ss->sgs.active_players[0] = e.current_active_players[0];

	  e.current_active_players[1] =
			  s->ps[(ss->last_active_player_index + 1) % ss->sgs.num_players]
					  .id;
	  ss->sgs.active_players[1] = e.current_active_players[1];

	  e.current_active_players[2] =
			  s->ps[(ss->last_active_player_index + 2) % ss->sgs.num_players]
					  .id;
	  ss->sgs.active_players[2] = e.current_active_players[2];

	  ss->last_active_player_index =
			  (ss->last_active_player_index + 1) % ss->sgs.num_players;

	  card_collection_empty(&ss->stiche_buf[0]);
	  card_collection_empty(&ss->stiche_buf[1]);
	  card_collection_empty(&ss->stiche_buf[2]);
	  ss->sgs.curr_stich = (stich){.vorhand = 0, .winner = -1};
	  ss->sgs.last_stich = (stich){.vorhand = -1, .winner = -1};
	  ss->sgs.stich_num = 0;

	  server_distribute_event(s, &e, NULL);

	  distribute_cards(ss);

	  e.type = EVENT_DISTRIBUTE_CARDS;

	  void mask_hands(event * ev, player * pl) {
		if (!is_active_player(&ss->sgs, pl)) {
		  card_collection_empty(&ev->hand);
		  return;
		}
		ev->hand = get_player_hand(ss, pl);
	  }

	  server_distribute_event(s, &e, mask_hands);

	  return GAME_PHASE_REIZEN_BEGIN;
	default:
	  return GAME_PHASE_INVALID;
  }
}

static game_phase
apply_action_reizen_begin(skat_state *ss, action *a, player *pl, server *s) {
  // remember to initialize stiche!
  event e;
  e.answer_to = a->id;
  e.player = pl->id;
  switch (a->type) {
	default:
	  return GAME_PHASE_INVALID;
  }
}

static game_phase
apply_action_stich(skat_state *ss, action *a, player *pl, server *s, int card) {
  event e;
  e.answer_to = a->id;
  e.player = pl->id;
  int cpi, result;
  switch (a->type) {
	case ACTION_PLAY_CARD:
	  cpi = (ss->sgs.curr_stich.vorhand + card) % 3;
	  if (!player_id_equals(&pl->id, &ss->sgs.active_players[cpi]))
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

	  ss->sgs.last_stich = ss->sgs.curr_stich;
	  ss->sgs.curr_stich =
			  (stich){.vorhand = ss->sgs.last_stich.winner, .winner = -1};

	  server_distribute_event(s, &e, NULL);

	  if (ss->sgs.stich_num++ < 9)
		return GAME_PHASE_PLAY_STICH_C1;

	  return GAME_PHASE_BETWEEN_ROUNDS;
	default:
	  return GAME_PHASE_INVALID;
  }
}

static game_phase
apply_action(skat_state *ss, action *a, player *pl, server *s) {
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
skat_state_apply(skat_state *ss, action *a, player *pl, server *s) {
  game_phase new;
  new = apply_action(ss, a, pl, s);
  if (new == GAME_PHASE_INVALID)
	return 0;
  ss->sgs.cgphase = new;
  return 1;
}

void
skat_state_tick(skat_state *ss, server *s) {}

void
skat_resync_player(skat_client_state *cs, player *pl) {
  // TODO: this
}

void
skat_state_init(skat_state *ss) {
  // TODO
}
