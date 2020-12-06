#include "client/console_input.h"
#include "client/card_printer.h"
#include "client/game_rules_printer.h"
#include "skat/card.h"
#include "skat/command.h"
#include "skat/game_rules.h"
#include "skat/player.h"
#include "skat/util.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define player_name_lookup_gupid(gupid) \
  (gupid) == -1 ? "ERROR" \
				: ((gupid) == c->cs.my_gupid \
						   ? "YOU" \
						   : (c->pls[(gupid)] ? c->pls[(gupid)]->name \
											  : "DISCONNECTED"))
#define player_name_lookup_ap(ap) \
  (ap) == -1 \
		  ? "ERROR" \
		  : ((ap) == c->cs.my_active_player_index \
					 ? "YOU" \
					 : (c->pls[c->cs.sgs.active_players[(ap)]] \
								? c->pls[c->cs.sgs.active_players[(ap)]]->name \
								: "DISCONNECTED"))

static void
print_players(const client *const c) {
  switch (c->cs.sgs.cgphase) {
	case GAME_PHASE_INVALID:
	  break;
	case GAME_PHASE_SETUP:
	  printf("Current players:\n");
	  for (int gupid = 0; gupid < 4; gupid++) {
		player *pl = c->pls[gupid];
		if (pl != NULL)
		  printf("\t%s[gupid=%d]\n", player_name_lookup_gupid(gupid), gupid);
	  }
	  break;
	case GAME_PHASE_BETWEEN_ROUNDS:
	  printf("Current players:\n");
	  for (int gupid = 0; gupid < 4; gupid++) {
		player *pl = c->pls[gupid];
		if (pl != NULL)
		  printf("\t%s[gupid=%d]: %ld\n", player_name_lookup_gupid(gupid),
				 gupid, c->cs.sgs.score[gupid]);
	  }
	  break;
	default:
	  printf("Current total player scores in seating order:\n");
	  for (int ap = 0; ap < 3; ap++) {
		int8_t gupid = c->cs.sgs.active_players[ap];
		printf("\t%s[gupid=%d, ap=%d]: %ld\n", player_name_lookup_ap(ap), gupid,
			   ap, c->cs.sgs.score[gupid]);
	  }
	  printf("Current total spectator scores:\n");
	  for (int gupid = 0; gupid < 4; gupid++) {
		player *pl = c->pls[gupid];
		if (pl != NULL && pl->ap == -1)
		  printf("\t%s[gupid=%d]: %ld\n", player_name_lookup_gupid(gupid),
				 gupid, c->cs.sgs.score[gupid]);
	  }
	  break;
  }
}

static void
print_player_turn(const client *const c,
				  const print_player_turn_show_hand_mode mode) {
  int8_t player_turn =
		  c->cs.sgs.active_players[(c->cs.sgs.curr_stich.vorhand
									+ c->cs.sgs.curr_stich.played_cards)
								   % 3];
  int is_my_turn = c->cs.my_gupid == player_turn;
  if (is_my_turn)
	printf("It is YOUR turn.");
  else
	printf("It is %s's turn.", player_name_lookup_gupid(player_turn));

  if ((is_my_turn && mode != PRINT_PLAYER_TURN_SHOW_HAND_MODE_NEVER)
	  || mode == PRINT_PLAYER_TURN_SHOW_HAND_MODE_ALWAYS) {
	printf(" Your cards:");
	print_card_collection(&c->cs.sgs, &c->cs.my_hand,
						  CARD_SORT_MODE_INGAME_HAND,
						  is_my_turn ? CARD_COLOR_MODE_PLAYABLE
									 : CARD_COLOR_MODE_ONLY_CARD_COLOR);
  }
}

static void
print_reizen_info(client *c, event *e) {
  reiz_state *rs = &c->cs.sgs.rs;

  if (e != NULL) {
	if (e->type == EVENT_DISTRIBUTE_CARDS)
	  printf("Reizen begin!\n");
	else if (e->type == EVENT_REIZEN_DONE)
	  printf("Reizen done at reizwert %u!\n", rs->reizwert);
  }

  int8_t teller_gupid, listener_gupid;
  if (rs->rphase == REIZ_PHASE_INVALID || rs->rphase == REIZ_PHASE_DONE) {
	teller_gupid = listener_gupid = -1;
  } else if (rs->rphase == REIZ_PHASE_MITTELHAND_TO_VORHAND) {
	teller_gupid = c->cs.sgs.active_players[1];
	listener_gupid = c->cs.sgs.active_players[0];
  } else if (rs->rphase == REIZ_PHASE_HINTERHAND_TO_WINNER) {
	teller_gupid = c->cs.sgs.active_players[2];
	listener_gupid = c->cs.sgs.active_players[rs->winner];
  } else if (rs->rphase == REIZ_PHASE_WINNER) {
	teller_gupid = c->cs.sgs.active_players[rs->winner];
	listener_gupid = -1;
  } else {
	__builtin_unreachable();
  }

  int is_actor = e != NULL && c->cs.my_gupid == e->acting_player;
  const char *actor_name =
		  player_name_lookup_gupid(e == NULL ? -1 : e->acting_player);

  int is_teller = c->cs.my_gupid == teller_gupid;
  const char *teller_name = player_name_lookup_gupid(teller_gupid);

  int is_listener = c->cs.my_gupid == listener_gupid;
  const char *listener_name = player_name_lookup_gupid(listener_gupid);

  if (e != NULL) {
	switch (e->type) {
	  case EVENT_DISTRIBUTE_CARDS:
	  case EVENT_REIZEN_DONE:
		break;
	  case EVENT_REIZEN_NUMBER:
		printf("%s reizt %u.\n", actor_name, rs->reizwert);
		break;
	  case EVENT_REIZEN_CONFIRM:
		printf("%s confirmed the reizwert %u.\n", actor_name, rs->reizwert);
		break;
	  case EVENT_REIZEN_PASSE:
		printf("%s %s gepasst at reizwert %u.\n", actor_name,
			   is_actor ? "hast" : "hat", rs->reizwert);
		break;
	  default:
		printf("ERROR: Invalid event type for reizen %s\n",
			   event_name_table[e->type]);
		break;
	}
  }

  if (rs->rphase == REIZ_PHASE_MITTELHAND_TO_VORHAND
	  || rs->rphase == REIZ_PHASE_HINTERHAND_TO_WINNER) {
	if (is_teller) {
	  printf("YOU are saying, %s is listening.\n", listener_name);
	  if (rs->waiting_teller)
		printf("It is YOUR turn. Go higher than %u or pass.\n", rs->reizwert);
	  else
		printf("Waiting for listener to confirm or pass at %u.\n",
			   rs->reizwert);
	} else if (is_listener) {
	  printf("%s is saying, YOU are listening.\n", teller_name);
	  if (rs->waiting_teller)
		printf("Waiting for teller to go higher than %u or pass.\n",
			   rs->reizwert);
	  else
		printf("It is YOUR turn to confirm or pass at %u.\n", rs->reizwert);
	} else {
	  printf("%s is saying, %s is listening.\n", teller_name, listener_name);
	  if (rs->waiting_teller)
		printf("Waiting for teller to go higher than %u or pass.\n",
			   rs->reizwert);
	  else
		printf("Waiting for listener to confirm or pass at %u.\n",
			   rs->reizwert);
	}
  } else if (rs->rphase == REIZ_PHASE_WINNER) {
	printf("Both players haben gepasst...\n");
	if (is_teller)
	  printf("Do YOU want to play, or do YOU want to ramsch?\n");
	else
	  printf("%s is deciding whether to play or ramsch.\n", teller_name);
  }

  if (e != NULL && e->type == EVENT_REIZEN_DONE) {
	if (c->cs.my_active_player_index != -1) {
	  if (c->cs.ist_alleinspieler) {
		printf("YOU are playing alone.\n");
		if (c->cs.sgs.gr.type != GAME_TYPE_RAMSCH) {
		  printf("Take or leave the skat.\n");
		} else {
		  // TODO: implement schieberamsch
		  printf("Ramsch time!\n");
		  print_player_turn(c, PRINT_PLAYER_TURN_SHOW_HAND_MODE_DEFAULT);
		  printf("\n");
		}
	  } else {
		printf("YOU are playing with %s while %s is playing alone.\n",
			   player_name_lookup_ap(c->cs.my_partner),
			   player_name_lookup_ap(c->cs.sgs.alleinspieler));
		printf("Waiting for %s to take or leave the skat.\n",
			   player_name_lookup_ap(c->cs.sgs.alleinspieler));
	  }
	} else {// spectator
	  if (c->cs.sgs.gr.type == GAME_TYPE_RAMSCH) {
		printf("Ramsch time!\n");
		print_player_turn(c, PRINT_PLAYER_TURN_SHOW_HAND_MODE_DEFAULT);
		printf("\n");
	  } else {
		printf("%s is playing alone.\n",
			   player_name_lookup_ap(c->cs.sgs.alleinspieler));
		printf("Waiting for %s to take or leave the skat.\n",
			   player_name_lookup_ap(c->cs.sgs.alleinspieler));
	  }
	}
  }
}

static void
print_info_exec(void *p) {
  client *c = p;

  client_acquire_state_lock(c);

  game_phase phase = c->cs.sgs.cgphase;
  stich *last_stich = &c->cs.sgs.last_stich;
  stich *stich = &c->cs.sgs.curr_stich;

  printf("\n--------------------------\n");

  printf("You are %s[gupid=%d, ap=%d]\n", c->pls[c->cs.my_gupid]->name,
		 c->cs.my_gupid, c->cs.my_active_player_index);

  if (c->cs.sgs.cgphase != GAME_PHASE_INVALID
	  && c->cs.sgs.cgphase != GAME_PHASE_SETUP
	  && c->cs.sgs.cgphase != GAME_PHASE_BETWEEN_ROUNDS
	  && c->cs.my_active_player_index == -1) {
	printf("You are spectating\n");
  }

  print_players(c);

  printf("Game Phase: %s\n", game_phase_name_table[phase]);

  if (phase == GAME_PHASE_REIZEN) {
	if (c->cs.my_active_player_index != -1) {
	  printf("Your hand:");
	  print_card_collection(&c->cs.sgs, &c->cs.my_hand,
							CARD_SORT_MODE_PREGAME_HAND,
							CARD_COLOR_MODE_ONLY_CARD_COLOR);
	}
	printf("\nrphase=%s, waiting_teller=%d, reizwert=%u, winner=%d\n",
		   reiz_phase_name_table[c->cs.sgs.rs.rphase],
		   c->cs.sgs.rs.waiting_teller, c->cs.sgs.rs.reizwert,
		   c->cs.sgs.rs.winner);
	print_reizen_info(c, NULL);
	printf("\n");
  } else if (phase == GAME_PHASE_SKAT_AUFNEHMEN) {
	if (c->cs.my_active_player_index == -1) {
	  printf("%s is playing alone\n",
			 player_name_lookup_ap(c->cs.sgs.alleinspieler));
	} else {
	  if (c->cs.ist_alleinspieler) {
		printf("You are playing alone\n");
	  } else {
		printf("You are playing with %s\n",
			   player_name_lookup_ap(c->cs.my_partner));
	  }
	}

	if (c->cs.my_active_player_index != -1) {
	  printf(" Your cards:");
	  print_card_collection(&c->cs.sgs, &c->cs.my_hand,
							CARD_SORT_MODE_PREGAME_HAND,
							CARD_COLOR_MODE_ONLY_CARD_COLOR);
	  printf("\n");
	}
  } else if (phase == GAME_PHASE_SPIELANSAGE) {
	if (c->cs.my_active_player_index == -1) {
	  printf("%s is playing alone\n",
			 player_name_lookup_ap(c->cs.sgs.alleinspieler));
	} else {
	  if (c->cs.ist_alleinspieler) {
		printf("You are playing alone\n");
	  } else {
		printf("You are playing with %s\n",
			   player_name_lookup_ap(c->cs.my_partner));
	  }
	}

	if (c->cs.my_active_player_index != -1) {
	  printf(" Your cards:");
	  print_card_collection(&c->cs.sgs, &c->cs.my_hand,
							CARD_SORT_MODE_PREGAME_HAND,
							CARD_COLOR_MODE_ONLY_CARD_COLOR);
	  printf("\n");
	}
  } else if (phase == GAME_PHASE_PLAY_STICH_C1
			 || phase == GAME_PHASE_PLAY_STICH_C2
			 || phase == GAME_PHASE_PLAY_STICH_C3) {
	print_game_rules_info(&c->cs.sgs.gr);

	if (c->cs.my_active_player_index == -1) {
	  printf("%s is playing alone\n",
			 player_name_lookup_ap(c->cs.sgs.alleinspieler));
	} else {
	  if (c->cs.ist_alleinspieler) {
		printf("You are playing alone\n");
	  } else {
		printf("You are playing with %s\n",
			   player_name_lookup_ap(c->cs.my_partner));
	  }
	}

	if (c->cs.sgs.stich_num > 0) {
	  printf("Last Stich (vorhand=%s, winner=%s):",
			 player_name_lookup_ap(last_stich->vorhand),
			 player_name_lookup_ap(last_stich->winner));
	  print_card_array(&c->cs.sgs, NULL, last_stich->cs,
					   last_stich->played_cards,
					   CARD_COLOR_MODE_ONLY_CARD_COLOR);
	  printf("\n");
	}
	printf("Current Stich (num=%d, vorhand=%s):", c->cs.sgs.stich_num,
		   player_name_lookup_ap(stich->vorhand));
	print_card_array(&c->cs.sgs, NULL, stich->cs, stich->played_cards,
					 CARD_COLOR_MODE_ONLY_CARD_COLOR);
	printf("\n");

	print_player_turn(c, c->cs.my_active_player_index == -1
								 ? PRINT_PLAYER_TURN_SHOW_HAND_MODE_NEVER
								 : PRINT_PLAYER_TURN_SHOW_HAND_MODE_ALWAYS);
	printf("\n");
  }

  printf("--------------------------\n> ");
  fflush(stdout);

  client_release_state_lock(c);
}

static void
client_reizen_callback(void *v) {
  client_reizen_callback_args *crca;
  crca = v;
  client *c = crca->hdr.c;

  if (crca->hdr.e.type == EVENT_ILLEGAL_ACTION) {
	printf("--\nBig unluck! You tried do a reizen, but that didn't work. "
		   "Better luck next time\n> ");
	goto end;
  }

  printf("--\n");

  client_acquire_state_lock(c);

  print_reizen_info(c, &crca->hdr.e);

  printf("> ");
  client_release_state_lock(c);
end:
  fflush(stdout);
  free(crca);// tree that
}

static void
execute_reizen_wrapper(void *v) {
  exec_reizen_wrapper_args *args = v;
  client_action_callback cac;
  client_reizen_callback_args *crca;
  crca = malloc(sizeof(*crca));
  cac.args = crca;
  cac.f = client_reizen_callback;

  switch (args->rit) {
	case REIZEN_INVALID:
	  DERROR_PRINTF("received invalid reizen input type");
	  assert(0);
	  break;
	case REIZEN_CONFIRM:
	  client_reizen_confirm(args->c, &cac);
	  break;
	case REIZEN_PASSE:
	  client_reizen_passe(args->c, &cac);
	  break;
	case REIZEN_NEXT:
	  client_reizen_number(args->c, 0, &cac);
	  break;
	default:
	  client_reizen_number(args->c, args->rit - REIZEN_VALUE_BASE, &cac);
  }

  free(args);
}

static void
execute_reizen(client *c, reizen_input_type rit) {
  async_callback acb;
  exec_reizen_wrapper_args *args;

  args = malloc(sizeof(exec_reizen_wrapper_args));
  args->c = c;
  args->rit = rit;

  acb = (async_callback){.do_stuff = execute_reizen_wrapper, .data = args};

  exec_async(&c->acq, &acb);
}

static void
client_skat_callback(void *v) {
  client_skat_callback_args *csca;
  csca = v;
  client *c = csca->hdr.c;

  if (csca->hdr.e.type == EVENT_ILLEGAL_ACTION) {
	printf("--\nBig unluck! You tried to modify the skat, but that was "
		   "illegal. Dirty cheater!\n> ");
	goto end;
  }

  printf("--\n");

  client_acquire_state_lock(c);

  switch (csca->hdr.e.type) {
	case EVENT_SKAT_TAKE:
	  printf("YOU took the skat. It contained:");
	  print_card_array(&csca->hdr.c->cs.sgs, NULL, csca->hdr.e.skat, 2,
					   CARD_COLOR_MODE_ONLY_CARD_COLOR);
	  printf("\nYour hand:");
	  print_card_collection(&csca->hdr.c->cs.sgs, &c->cs.my_hand,
							CARD_SORT_MODE_PREGAME_HAND,
							CARD_COLOR_MODE_ONLY_CARD_COLOR);
	  break;
	case EVENT_SKAT_LEAVE:
	  printf("YOU left the skat.\n");
	  printf("Please make your Spielansage mit Hand");
	  break;
	case EVENT_SKAT_PRESS:
	  printf("YOU pressed:");
	  print_card_array(&csca->hdr.c->cs.sgs, NULL, csca->hdr.e.skat_press_cards,
					   2, CARD_COLOR_MODE_ONLY_CARD_COLOR);
	  printf("\nPlease make your Spielansage ohne Hand");
	  break;
	default:
	  DERROR_PRINTF("received invalid skat event");
	  assert(0);
	  break;
  }

  printf("\n> ");

  client_release_state_lock(c);
end:
  fflush(stdout);
  free(csca);
}

static void
execute_skat_wrapper(void *v) {
  exec_skat_wrapper_args *args = v;
  client_action_callback cac;
  client_skat_callback_args *csca;
  csca = malloc(sizeof(*csca));
  cac.args = csca;
  cac.f = client_skat_callback;

  switch (args->sit) {
	case SKAT_TAKE:
	  client_skat_take(args->c, &cac);
	  break;
	case SKAT_LEAVE:
	  client_skat_leave(args->c, &cac);
	  break;
	case SKAT_PRESS:
	  client_skat_press(args->c, args->cid1, args->cid2, &cac);
	  break;
	default:
	  DERROR_PRINTF("received invalid skat input type");
	  assert(0);
	  break;
  }

  free(args);
}

static void
execute_skat(client *c, skat_input_type sit, card_id cid1, card_id cid2) {
  async_callback acb;
  exec_skat_wrapper_args *args;

  args = malloc(sizeof(exec_skat_wrapper_args));
  args->c = c;
  args->sit = sit;
  args->cid1 = cid1;
  args->cid2 = cid2;

  acb = (async_callback){.do_stuff = execute_skat_wrapper, .data = args};

  exec_async(&c->acq, &acb);
}

static void
client_ready_callback(void *v) {
  client_ready_callback_args *args = v;
  client *c = args->hdr.c;
  event *e = &args->hdr.e;

  client_acquire_state_lock(c);

  printf("--\n");
  if (e->type == EVENT_ILLEGAL_ACTION) {
	printf("Big unluck! You tried to ready yourself, but that was illegal\n");
  } else if (e->type == EVENT_START_GAME) {
	printf("YOU started the game\n");

	printf("Participating players:\n");
	for (int gupid = 0; gupid < 4; gupid++) {
	  player *pl = c->pls[gupid];
	  if (pl != NULL)
		printf(" - %s\n", player_name_lookup_gupid(gupid));
	}
  } else if (e->type == EVENT_START_ROUND) {
	printf("YOU started the round\n");

	if (c->cs.my_active_player_index == -1)
	  printf("YOU are spectating\n");
	else
	  printf("YOU are playing\n");

	print_players(c);
  } else {
	printf("ERROR: Received invalid event type %s after ready\n",
		   event_name_table[e->type]);
  }

  printf("> ");
  fflush(stdout);
  client_release_state_lock(c);
  free(args);
}

static void
execute_ready_wrapper(void *v) {
  client *c = v;
  client_action_callback cac;
  client_ready_callback_args *cach = malloc(sizeof(client_ready_callback_args));
  cac.args = cach;
  cac.f = client_ready_callback;
  client_ready(c, &cac);
}

static void
execute_ready(client *c) {
  async_callback acb;

  acb = (async_callback){.do_stuff = execute_ready_wrapper, .data = c};

  exec_async(&c->acq, &acb);
}

/* Begin execute set gamerules logic
   -------------------------------- */

static void
client_set_gamerules_callback(void *v) {
  __label__ end;
  client_set_gamerules_callback_args *args = v;

  client_acquire_state_lock(args->hdr.c);

  printf("--\n");

  if (args->hdr.e.type == EVENT_ILLEGAL_ACTION) {
	printf("You tried to cheat by calling an illegal game! Luckily for you, "
		   "we "
		   "didn't insta-loose the game for you");
	goto end;
  }

  print_game_rules_info(&args->hdr.c->cs.sgs.gr);
  printf("\nThe game is on.\n");

  print_player_turn(args->hdr.c, PRINT_PLAYER_TURN_SHOW_HAND_MODE_DEFAULT);
  printf("\n");

end:
  printf("> ");
  fflush(stdout);
  client_release_state_lock(args->hdr.c);
  free(args);// TODO: Prevent malloc thread drift by using a custom allocator
			 // for these queues
}

static void
client_set_gamerules_wrapper(void *v) {
  struct client_set_gamerules_args *args = v;
  client_action_callback cac;

  client_set_gamerules_callback_args *cach =
		  malloc(sizeof(client_set_gamerules_callback_args));
  cac.args = cach;
  cac.f = client_set_gamerules_callback;
  client_set_gamerules(args->c, args->gr, &cac);
  free(args);
}

static void
execute_set_gamerules(client *c, game_rules *gr) {
  async_callback acb;

  struct client_set_gamerules_args *args =
		  malloc(sizeof(struct client_set_gamerules_args));
  args->c = c;
  args->gr = *gr;

  acb = (async_callback){.do_stuff = client_set_gamerules_wrapper,
						 .data = args};

  exec_async(&c->acq, &acb);
}

/* --------------------------------
   End execute set gamerules logic */


/* Begin execute play card logic
   -------------------------------- */

static void
client_play_card_callback(void *v) {
  client_play_card_callback_args *args = v;

  client_acquire_state_lock(args->hdr.c);

  printf("--\n");

  if (args->hdr.e.type == EVENT_ILLEGAL_ACTION) {
	printf("Big anlak! You tried to play a card, but it -sadly- was the wrong "
		   "card");
	goto end;
  }

  char buf[4];
  card_get_name(&args->hdr.e.card, buf);
  printf("Successfully played card %s. Cards currently on table:", buf);
  if (args->hdr.c->cs.sgs.curr_stich.played_cards > 0
	  && args->hdr.c->cs.sgs.curr_stich.played_cards
				 < 3) {// 1 or 2 cards played
	print_card_array(&args->hdr.c->cs.sgs, NULL,
					 args->hdr.c->cs.sgs.curr_stich.cs,
					 args->hdr.c->cs.sgs.curr_stich.played_cards,
					 CARD_COLOR_MODE_ONLY_CARD_COLOR);
	printf("\n");
	print_player_turn(args->hdr.c, PRINT_PLAYER_TURN_SHOW_HAND_MODE_DEFAULT);
  } else {// 3 cards played, player turn info printed on EVENT_STICH_DONE
	print_card_array(&args->hdr.c->cs.sgs, NULL,
					 args->hdr.c->cs.sgs.last_stich.cs,
					 args->hdr.c->cs.sgs.last_stich.played_cards,
					 CARD_COLOR_MODE_ONLY_CARD_COLOR);
  }

end:
  printf("\n> ");
  fflush(stdout);
  client_release_state_lock(args->hdr.c);
  free(args);
}

static void
client_play_card_wrapper(void *v) {
  struct client_play_card_args *args = v;
  client_action_callback cac;

  client_play_card_callback_args *cach =
		  malloc(sizeof(client_play_card_callback_args));
  cac.args = cach;
  cac.f = client_play_card_callback;
  client_play_card(args->c, args->cid, &cac);
  free(args);
}

static void
execute_play_card(client *c, card_id cid) {
  async_callback acb;

  struct client_play_card_args *args =
		  malloc(sizeof(struct client_play_card_args));
  args->c = c;
  args->cid = cid;

  acb = (async_callback){.do_stuff = client_play_card_wrapper, .data = args};

  exec_async(&c->acq, &acb);
}

/* --------------------------------
   End execute play card logic */

static void
print_ramsch_announce(client *c, event *e) {
  printf("This round of Ramschen is over.\n");

  if (e->rr.round_winner != -1) {// there is a specific winner
	const char *winner_name = player_name_lookup_ap(e->rr.round_winner);
	if ((loss_type) e->rr.loss_type == LOSS_TYPE_WON_DURCHMARSCH)
	  printf("%s won this round in a durchmarsch\n", winner_name);
	else
	  printf("ERROR: %s somehow won a ramsch without durchmarsch?\n",
			 winner_name);
  }

  for (int i = 0; i < 3; ++i) {
	printf("%s got %d points with %u cards\n", player_name_lookup_ap(i),
		   e->rr.player_points[i], e->rr.player_stich_card_count[i]);
  }

  printf("\nScores for current round:\n");
  for (int i = 0; i < 3; ++i) {
	printf("\t%s: %ld\n", player_name_lookup_ap(i), e->rr.round_score[i]);
  }
}

static void
print_event_announce(client *c, event *e) {
  if (c->cs.sgs.gr.type == GAME_TYPE_RAMSCH) {
	print_ramsch_announce(c, e);
	return;
  }

  printf("This round is over. The Spielwert was %d.\n", e->rr.spielwert);

  int alleinspieler_is_winner = e->rr.round_winner != -1;
  const char *winner1_name = NULL, *winner2_name = NULL, *loser1_name = NULL,
			 *loser2_name = NULL;
  int8_t winner1_ap = -1, winner2_ap = -1, loser1_ap = -1, loser2_ap = -1;
  if (alleinspieler_is_winner) {// the alleinspieler won
	winner1_ap = c->cs.sgs.alleinspieler;
	switch (c->cs.sgs.alleinspieler) {
	  case 0:
		loser1_ap = 1;
		loser2_ap = 2;
		break;
	  case 1:
		loser1_ap = 0;
		loser2_ap = 2;
		break;
	  case 2:
		loser1_ap = 0;
		loser2_ap = 1;
		break;
	  default:
		break;
	}
  } else {// the gegenpartei won
	loser1_ap = c->cs.sgs.alleinspieler;
	switch (c->cs.sgs.alleinspieler) {
	  case 0:
		winner1_ap = 1;
		winner2_ap = 2;
		break;
	  case 1:
		winner1_ap = 0;
		winner2_ap = 2;
		break;
	  case 2:
		winner1_ap = 0;
		winner2_ap = 1;
		break;
	  default:
		break;
	}
  }

  winner1_name = player_name_lookup_ap(winner1_ap);
  winner2_name = player_name_lookup_ap(winner2_ap);
  loser1_name = player_name_lookup_ap(loser1_ap);
  loser2_name = player_name_lookup_ap(loser2_ap);

  if (winner2_ap == -1)
	printf("%s won this round, while %s and %s lost\n", winner1_name,
		   loser1_name, loser2_name);
  else
	printf("%s and %s won this round, while %s lost\n", winner1_name,
		   winner2_name, loser1_name);

  if (winner2_ap == -1) {// the alleinspieler won
	printf("The Alleinspieler (%s) got %d points with %u cards\n", winner1_name,
		   e->rr.player_points[winner1_ap],
		   e->rr.player_stich_card_count[winner1_ap]);
	printf("The Gegenpartei (%s and %s) got %d points with %u cards\n",
		   loser1_name, loser2_name, e->rr.player_points[loser1_ap],
		   e->rr.player_stich_card_count[loser1_ap]);

	if (e->rr.schwarz)
	  printf("The Gegenpartei was played schwarz\n");
	else if (e->rr.schneider)
	  printf("The Gegenpartei was played into schneider\n");
  } else {// the gegenpartei won
	printf("The Gegenpartei (%s and %s) got %d points with %u cards\n",
		   winner1_name, winner2_name, e->rr.player_points[winner1_ap],
		   e->rr.player_stich_card_count[winner1_ap]);
	printf("The Alleinspieler (%s) got %d points with %u cards\n", loser1_name,
		   e->rr.player_points[loser1_ap],
		   e->rr.player_stich_card_count[loser1_ap]);
  }

  printf("\nScores for the this round:\n");
  for (int i = 0; i < 3; ++i) {
	printf("\t%s: %ld\n", player_name_lookup_ap(i), e->rr.round_score[i]);
  }
}

static void
execute_print_info(client *c) {
  async_callback acb;

  acb = (async_callback){.do_stuff = print_info_exec, .data = c};

  exec_async(&c->acq, &acb);
}

void
io_handle_event(client *c, event *e) {
  char buf[4];

  printf("--\n");

  client_acquire_state_lock(c);

  switch (e->type) {
	case EVENT_START_GAME:
	  printf("The game was started by %s\n",
			 player_name_lookup_gupid(e->acting_player));

	  printf("Participating players:\n");
	  for (int gupid = 0; gupid < 4; gupid++) {
		player *pl = c->pls[gupid];
		if (pl != NULL)
		  printf(" - %s\n", player_name_lookup_gupid(gupid));
	  }
	  break;
	case EVENT_START_ROUND:
	  printf("The round was started by %s\n",
			 player_name_lookup_gupid(e->acting_player));

	  if (c->cs.my_active_player_index == -1)
		printf("YOU are spectating\n");
	  else
		printf("YOU are playing\n");

	  print_players(c);
	  break;
	case EVENT_DISTRIBUTE_CARDS:
	  if (c->cs.my_active_player_index != -1) {
		printf("Your hand:");
		print_card_collection(&c->cs.sgs, &c->cs.my_hand,
							  CARD_SORT_MODE_PREGAME_HAND,
							  CARD_COLOR_MODE_ONLY_CARD_COLOR);
		printf("\n");
	  }
	  print_reizen_info(c, e);
	  break;
	case EVENT_REIZEN_NUMBER:
	case EVENT_REIZEN_CONFIRM:
	case EVENT_REIZEN_PASSE:
	case EVENT_REIZEN_DONE:
	  print_reizen_info(c, e);
	  break;
	case EVENT_SKAT_TAKE:
	  printf("%s took the skat.\n", player_name_lookup_gupid(e->acting_player));
	  printf("Waiting for press.\n");
	  break;
	case EVENT_SKAT_LEAVE:
	  printf("%s left the skat.\n", player_name_lookup_gupid(e->acting_player));
	  printf("Waiting for Spielansage mit Hand.\n");
	  break;
	case EVENT_SKAT_PRESS:
	  printf("%s pressed two cards.\n",
			 player_name_lookup_gupid(e->acting_player));
	  printf("Waiting for Spielansage ohne Hand.\n");
	  break;
	case EVENT_GAME_CALLED:
	  print_game_rules_info(&c->cs.sgs.gr);
	  print_player_turn(c, PRINT_PLAYER_TURN_SHOW_HAND_MODE_DEFAULT);
	  printf("\n");
	  break;
	case EVENT_PLAY_CARD:
	  card_get_name(&e->card, buf);
	  printf("Card %s played. Cards currently on table: ", buf);
	  if (c->cs.sgs.curr_stich.played_cards > 0
		  && c->cs.sgs.curr_stich.played_cards < 3) {// 1 or 2 cards played
		print_card_array(&c->cs.sgs, NULL, c->cs.sgs.curr_stich.cs,
						 c->cs.sgs.curr_stich.played_cards,
						 CARD_COLOR_MODE_ONLY_CARD_COLOR);
		printf("\n");
		print_player_turn(c, PRINT_PLAYER_TURN_SHOW_HAND_MODE_DEFAULT);
		printf("\n");
	  } else {// 3 cards played, player turn info printed on EVENT_STICH_DONE
		print_card_array(&c->cs.sgs, NULL, c->cs.sgs.last_stich.cs,
						 c->cs.sgs.last_stich.played_cards,
						 CARD_COLOR_MODE_ONLY_CARD_COLOR);
		printf("\n");
	  }
	  break;
	case EVENT_STICH_DONE:
	  if (c->cs.my_active_player_index != -1 && !c->cs.ist_alleinspieler
		  && e->stich_winner == c->cs.sgs.active_players[c->cs.my_partner]) {
		printf("Your partner %s won the Stich.\n",
			   player_name_lookup_gupid(e->stich_winner));
	  } else {
		printf("%s won the Stich.\n",
			   player_name_lookup_gupid(e->stich_winner));
	  }

	  if (c->cs.sgs.stich_num < 10
		  && (c->cs.sgs.gr.type != GAME_TYPE_NULL
			  || e->stich_winner
						 != c->cs.sgs
									.active_players[c->cs.sgs.alleinspieler])) {
		print_player_turn(c, PRINT_PLAYER_TURN_SHOW_HAND_MODE_DEFAULT);
		printf("\n");
	  }
	  break;
	case EVENT_ANNOUNCE_SCORES:
	  print_event_announce(c, e);
	  break;
	case EVENT_ROUND_DONE:
	  print_players(c);
	  break;
	default:
	  printf("Something (%s) happened\n", event_name_table[e->type]);
  }
  goto skip;
  // Ha, unreachable
skip:
  client_release_state_lock(c);
  printf("> ");
  fflush(stdout);
}

static void
stop_client(client *c) {
  client_acquire_state_lock(c);
  client_prepare_exit(c);
  client_release_state_lock(c);
  exit(EXIT_SUCCESS);
}

// Assuming cmd is commando, result is clobberable
#define MATCH_NUM_ARGS(command_name, num) \
  if (!command_check_arg_length(cmd, num, &result) && result)
#define MATCH_NUM_ARGS_END(command_name, num) \
  { \
	printf("Expected " #num " args for " #command_name " , but got %zu\n", \
		   cmd->args_length); \
	goto command_cleanup; \
  }

void *
handle_console_input(void *v) {
  client *c = v;
  char *line = NULL;
  size_t line_size = 0;

  DEBUG_PRINTF("Started console input thread");
  printf("Welcome to Skat. Type 'help' for help!\n");

  for (;;) {
	printf("> ");
	fflush(stderr);
	fflush(stdout);

	// getline uses realloc on the given buffer, thus free is not required
	ssize_t read = getline(&line, &line_size, stdin);

	if (read == EOF) {
	  printf("Read EOF\n");
	  fflush(stdout);
	  break;
	}

	command *cmd;
	int error = command_create(&cmd, line, read);
	if (error) {
	  printf("Invalid command (error %d): '%s'", error, line);
	  continue;
	}

	/*printf("command: %s\n", cmd->command);
	printf("%zu args%s\n", cmd->args_length, cmd->args_length > 0 ? ":" : "");
	for (size_t i = 0; i < cmd->args_length; i++)
	  printf("  %zu: %s\n", i, cmd->args[i]);*/

	int result = 0;
	uint16_t reizwert;

	// ready
	if (!command_equals(cmd, &result, 1, "ready") && result) {
	  MATCH_NUM_ARGS(ready, 0) { execute_ready(c); }
	  else MATCH_NUM_ARGS_END(ready, 0)
	}

	// reizen <"number" | next | (weg | passe) | ja>
	else if (!command_equals(cmd, &result, 1, "reizen") && result) {
	  MATCH_NUM_ARGS(reizen, 1) {
		if (!command_arg_equals(cmd, 0, 0, &result, 2, "ja", "j") && result)
		  execute_reizen(c, REIZEN_CONFIRM);
		else if (!command_arg_equals(cmd, 0, 0, &result, 5, "weg", "passe", "w",
									 "p", "nein")
				 && result)
		  execute_reizen(c, REIZEN_PASSE);
		else if (!command_arg_equals(cmd, 0, 0, &result, 2, "next", "n")
				 && result)
		  execute_reizen(c, REIZEN_NEXT);
		else if (!command_parse_arg_u16(cmd, 0, 0, 18, -1, &reizwert))
		  execute_reizen(c, REIZEN_VALUE_BASE + reizwert);
		else
		  printf("Usage: reizen <\"number\" | next | (weg | passe) | ja>\n");
	  }
	  else MATCH_NUM_ARGS_END(reizen, 1)
	}

	// TODO: print usage on error, smartly
	// skat <take | leave | press <cid1> <cid2>>
	else if (!command_equals(cmd, &result, 1, "skat") && result) {
	  MATCH_NUM_ARGS(reizen, 1) {
		if (!command_arg_equals(cmd, 0, 0, &result, 2, "take", "t") && result)
		  execute_skat(c, SKAT_TAKE, 0, 0);
		else if (!command_arg_equals(cmd, 0, 0, &result, 2, "leave", "l")
				 && result)
		  execute_skat(c, SKAT_LEAVE, 0, 0);
		else
		  printf("Usage: skat <take | leave | press <cid1> <cid2>\n");
	  }
	  else MATCH_NUM_ARGS(reizen, 3) {
		if (!command_arg_equals(cmd, 1, 0, &result, 2, "press", "p")
			&& result) {
		  card_id cid1, cid2;
		  if (!command_parse_arg_u8(cmd, 1, 1, 0, CARD_ID_MAX, &cid1)
			  && !command_parse_arg_u8(cmd, 1, 2, 0, CARD_ID_MAX, &cid2)) {
			execute_skat(c, SKAT_PRESS, cid1, cid2);
		  } else {
			printf("Usage: skat <take | leave | press <cid1> <cid2>\n");
		  }
		} else {
		  printf("Usage: skat <take | leave | press <card id 1> <card id "
				 "2>>\n");
		}
	  }
	  else MATCH_NUM_ARGS_END(reizen, "1 or 3")
	}

	// spiel <color> {modifier}
	else if (!command_equals(cmd, &result, 1, "spiel") && result) {
	  game_rules /*z?*/ gr;
	  memset(&gr, '\0', sizeof gr);
	  if (cmd->args_length < 1) {
		printf("Usage: spiel <color> {modifier}\n");
		goto command_cleanup;
	  }

	  if (!command_arg_equals(cmd, 0, 0, &result, 2, "null", "n") && result) {
		gr.type = GAME_TYPE_NULL;
		FOR_EACH_ARG_I(cmd, i, 1, {
		  if (!command_arg_equals(cmd, 0, i, &result, 2, "hand", "h")
			  && result) {
			gr.hand = 1;
		  } else if (!command_arg_equals(cmd, 0, i, &result, 4, "ouvert",
										 "overt", "ov", "o")
					 && result) {
			gr.ouvert = 1;
		  }
		});
	  }
	  /* XXX: */
	  else if ((!command_arg_equals(cmd, 0, 0, &result, 2, "grand", "g")
				&& result && (gr.type = GAME_TYPE_GRAND))
			   || (!command_arg_equals(cmd, 0, 0, &result, 2, "kreuz", "kr")
				   && result && (gr.type = GAME_TYPE_COLOR)
				   && (gr.trumpf = COLOR_KREUZ))
			   || (!command_arg_equals(cmd, 0, 0, &result, 2, "pik", "p")
				   && result && (gr.type = GAME_TYPE_COLOR)
				   && (gr.trumpf = COLOR_PIK))
			   || (!command_arg_equals(cmd, 0, 0, &result, 2, "herz", "h")
				   && result && (gr.type = GAME_TYPE_COLOR)
				   && (gr.trumpf = COLOR_HERZ))
			   || (!command_arg_equals(cmd, 0, 0, &result, 2, "karo", "ka")
				   && result && (gr.type = GAME_TYPE_COLOR)
				   && (gr.trumpf = COLOR_KARO))) {
		FOR_EACH_ARG_I(cmd, i, 1, {
		  if (!command_arg_equals(cmd, 0, i, &result, 4, "ouvert", "overt",
								  "ov", "o")
			  && result) {
			gr.ouvert = 1;
			goto schwarz;
		  } else if (!command_arg_equals(cmd, 0, i, &result, 2, "schwarz",
										 "schw")
					 && result) {
		  schwarz:
			gr.schwarz_angesagt = 1;
			goto schneider;
		  } else if (!command_arg_equals(cmd, 0, i, &result, 2, "schneider",
										 "schn")
					 && result) {
		  schneider:
			gr.schneider_angesagt = 1;
			goto hand;
		  } else if (!command_arg_equals(cmd, 0, i, &result, 2, "hand", "h")
					 && result) {
		  hand:
			gr.hand = 1;
		  } else {
			printf("Illegal modifier %s encountered in spiel command\n",
				   cmd->args[i]);
			goto command_cleanup;
		  }
		});
	  } else {
		printf("Unknown spiel type %s found\n", cmd->args[0]);
		goto command_cleanup;
	  }
	  execute_set_gamerules(c, &gr);
	  goto command_cleanup;
	}

	// play <card index>
	else if (!command_equals(cmd, &result, 1, "play") && result) {
	  if (command_check_arg_length(cmd, 1, &result) || !result) {
		printf("Expected exactly 1 arg for play, but got %zu\n",
			   cmd->args_length);
	  } else {
		card_id cid;
		if (!command_parse_arg_u8(cmd, 1, 0, 0, CARD_ID_MAX, &cid))
		  execute_play_card(c, cid);
	  }
	}

	// info
	else if (!command_equals(cmd, &result, 1, "info") && result) {
	  if (command_check_arg_length(cmd, 0, &result) || !result) {
		printf("Expected exactly 0 args for info, but got %zu\n",
			   cmd->args_length);
	  } else {
		execute_print_info(c);
	  }
	}

	// exit
	else if (!command_equals(cmd, &result, 2, "exit", "quit") && result) {
	  if (command_check_arg_length(cmd, 0, &result) || !result) {
		printf("Expected exactly 0 args for exit, but got %zu\n",
			   cmd->args_length);
	  } else {
		stop_client(c);
	  }
	}

	// help
	else if (!command_equals(cmd, &result, 2, "help", "?") && result) {
	  printf("Commands:\n");
	  printf("\thelp | print this help\n");
	  printf("\tready | start the game or the next round\n");
	  printf("\treizen <\"number\" | next | (weg | passe) | ja> | do a reizen "
			 "action\n");
	  printf("\tskat <take | leave | press <card id 1> <card id 2>> | do a "
			 "skat action\n");
	  printf("\tspiel <grand | kreuz | pik | herz | karo | null> "
			 "{modifiers...} | do the spielansage\n");
	  printf("\tplay <card id> | play a card\n");
	  printf("\tinfo | print information about the current state of the "
			 "game\n");
	  printf("\texit | exit the client\n");
	}

	// unknown command
	else {
	  printf("Unknown command: %s\nUse 'help' for list of commands\n",
			 cmd->command);
	}

  command_cleanup:
	command_free(cmd);
  }

  if (line)
	free(line);

  stop_client(c);

  __builtin_unreachable();
}
