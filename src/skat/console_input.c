#include "skat/console_input.h"
#include "skat/card.h"
#include "skat/card_collection.h"
#include "skat/client.h"
#include "skat/command.h"
#include "skat/util.h"
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static void
print_card_array(const card_id *const arr, const size_t length) {
  char buf[4];

  for (size_t i = 0; i < length; i++) {
	card_get_name(&arr[i], buf);
	printf(" %s(%d)", buf, arr[i]);
  }
}

typedef enum card_color_mode {
  CARD_COLOR_MODE_NONE,
  CARD_COLOR_MODE_ONLY_CARD_COLOR,
  CARD_COLOR_MODE_PLAYABLE
} card_color_mode;

#define RED_CARD_COLOR     "\e[31;1m"
#define BLACK_CARD_COLOR   "\e[30;1m"
#define PLAYABLE_COLOR     "\e[4m"
#define NOT_PLAYABLE_COLOR ""

static void
print_card_collection(const client *const c, const card_collection *const cc,
					  const card_sort_mode sort_mode,
					  const card_color_mode color_mode) {
  uint8_t count;
  if (card_collection_get_card_count(cc, &count))
	return;

  card_id *cid_array = malloc(count * sizeof(card_id));

  uint8_t j = 0;
  for (uint8_t i = 0; i < count; i++) {
	card_id cid;
	if (card_collection_get_card(cc, &i, &cid))
	  continue;

	cid_array[j++] = cid;
  }

  card_compare_args args =
		  (card_compare_args){.gr = &c->cs.sgs.gr, .mode = &sort_mode};

  qsort_r(cid_array, j, sizeof(card_id),
		  (int (*)(const void *, const void *, void *)) card_compare, &args);

  char buf[4];
  card card;
  for (uint8_t i = 0; i < j; i++) {
	card_id cid = cid_array[i];

	int error = card_get(&cid, &card);
	if (error)
	  continue;

	error = card_get_name(&cid, buf);
	if (error)
	  continue;

	if (color_mode == CARD_COLOR_MODE_PLAYABLE) {
	  int is_playable;
	  error = stich_card_legal(&c->cs.sgs.gr, &c->cs.sgs.curr_stich, &cid, cc,
							   &is_playable);
	  if (error)
		continue;

	  printf(" %s%s%s(%d)" COLOR_CLEAR,
			 is_playable ? PLAYABLE_COLOR : NOT_PLAYABLE_COLOR,
			 (card.cc == COLOR_KREUZ || card.cc == COLOR_PIK) ? BLACK_CARD_COLOR
															  : RED_CARD_COLOR,
			 buf, cid);
	} else if (color_mode == CARD_COLOR_MODE_ONLY_CARD_COLOR) {
	  printf(" %s%s(%d)" COLOR_CLEAR,
			 (card.cc == COLOR_KREUZ || card.cc == COLOR_PIK) ? BLACK_CARD_COLOR
															  : RED_CARD_COLOR,
			 buf, cid);
	} else {
	  printf(" %s(%d)", buf, cid);
	}
  }
}

typedef enum {
  PRINT_PLAYER_TURN_SHOW_HAND_MODE_ALWAYS,
  PRINT_PLAYER_TURN_SHOW_HAND_MODE_DEFAULT,
  PRINT_PLAYER_TURN_SHOW_HAND_MODE_NEVER
} print_player_turn_show_hand_mode;

static void
print_player_turn(const client *const c,
				  const print_player_turn_show_hand_mode mode) {
  int player_turn =
		  c->cs.sgs.active_players[(c->cs.sgs.curr_stich.vorhand
									+ c->cs.sgs.curr_stich.played_cards)
								   % 3];
  int is_my_turn = c->cs.my_gupid == player_turn;
  if (is_my_turn)
	printf("It is YOUR turn.");
  else
	printf("It is %s's turn.", c->pls[player_turn]->name);

  if ((is_my_turn && mode != PRINT_PLAYER_TURN_SHOW_HAND_MODE_NEVER)
	  || mode == PRINT_PLAYER_TURN_SHOW_HAND_MODE_ALWAYS) {
	printf(" Your cards:");
	print_card_collection(c, &c->cs.my_hand, CARD_SORT_MODE_HAND,
						  is_my_turn ? CARD_COLOR_MODE_PLAYABLE
									 : CARD_COLOR_MODE_ONLY_CARD_COLOR);
  }
}

static void
print_info_exec(void *p) {
  client *c = p;

  client_acquire_state_lock(c);

  game_phase phase = c->cs.sgs.cgphase;
  stich *last_stich = &c->cs.sgs.last_stich;
  stich *stich = &c->cs.sgs.curr_stich;
  card_collection *hand = &c->cs.my_hand;
  card_collection *won_stiche = &c->cs.my_stiche;

  printf("--\n\n--------------------------\n");

  printf("You are %s[gupid=%d, active_player=%d]\n",
		 c->pls[c->cs.my_gupid]->name, c->cs.my_gupid,
		 c->cs.my_active_player_index);

  printf("You are playing with:");
  for (int i = 0; i < 4; i++) {
	player *pl = c->pls[i];
	if (pl == NULL)
	  continue;
	printf(" %s[gupid=%d, active_player=%d]", pl->name, pl->gupid, pl->ap);
  }
  printf("\n");

  printf("Game Phase: %s, Type: %d, Trumpf: %d\n", game_phase_name_table[phase],
		 c->cs.sgs.gr.type, c->cs.sgs.gr.trumpf);

  if (phase == GAME_PHASE_PLAY_STICH_C1 || phase == GAME_PHASE_PLAY_STICH_C2
	  || phase == GAME_PHASE_PLAY_STICH_C3) {
	if (c->cs.ist_alleinspieler) {
	  printf("You are playing alone, the skat was:");
	  print_card_array(c->cs.skat, 2);
	  printf("\n");
	} else {
	  printf("You are playing with %s\n",
			 c->pls[c->cs.sgs.active_players[c->cs.my_partner]]->name);
	}

	if (c->cs.sgs.stich_num > 0) {
	  printf("Last Stich (num=%d, vorhand=%s, winner=%s):", c->cs.sgs.stich_num,
			 c->pls[c->cs.sgs.active_players[last_stich->vorhand]]->name,
			 c->pls[c->cs.sgs.active_players[last_stich->winner]]->name);
	  print_card_array(last_stich->cs, last_stich->played_cards);
	  printf("\n");
	}
	printf("Current Stich (num=%d, vorhand=%s):", c->cs.sgs.stich_num,
		   c->pls[c->cs.sgs.active_players[stich->vorhand]]->name);
	print_card_array(stich->cs, stich->played_cards);
	printf("\n");

	print_player_turn(c, PRINT_PLAYER_TURN_SHOW_HAND_MODE_NEVER);
	printf("\n");

	printf("Your hand:");
	print_card_collection(c, hand, CARD_SORT_MODE_HAND,
						  CARD_COLOR_MODE_PLAYABLE);
	printf("\n");

	unsigned int score;
	card_collection_get_score(won_stiche, &score);
	printf("Your stiche(score=%u):", score);
	print_card_collection(c, won_stiche, CARD_SORT_MODE_STICHE,
						  CARD_COLOR_MODE_ONLY_CARD_COLOR);
	printf("\n");
  }

  printf("--------------------------\n\n> ");
  fflush(stdout);

  client_release_state_lock(c);
}

typedef struct {
  client_action_callback_hdr hdr;
} client_ready_callback_args;

static void
client_ready_callback(void *v) {
  client_ready_callback_args *args = v;
  if (args->hdr.e.type == EVENT_ILLEGAL_ACTION)
	printf("--\nBig unluck! You tried to ready yourself, but that was "
		   "illegal\n> ");
  else
	printf("--\nReady for battle. We are now in %s\n> ",
		   game_phase_name_table[args->hdr.c->cs.sgs.cgphase]);
  fflush(stdout);
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

struct client_play_card_args {
  client *c;
  card_id cid;
};

typedef struct {
  client_action_callback_hdr hdr;
} client_play_card_callback_args;

static void
client_play_card_callback(void *v) {
  __label__ end;
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
  if (args->hdr.c->cs.sgs.curr_stich.played_cards > 0)
	print_card_array(args->hdr.c->cs.sgs.curr_stich.cs,
					 args->hdr.c->cs.sgs.curr_stich.played_cards);
  else
	print_card_array(args->hdr.c->cs.sgs.last_stich.cs,
					 args->hdr.c->cs.sgs.last_stich.played_cards);

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
execute_print_info(client *c) {
  async_callback acb;

  acb = (async_callback){.do_stuff = print_info_exec, .data = c};

  exec_async(&c->acq, &acb);
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

void
io_handle_event(client *c, event *e) {
  char buf[4];

  printf("--\n");
  switch (e->type) {
	case EVENT_DISTRIBUTE_CARDS:
	  printf("Your hand: ");
	  print_card_collection(c, &c->cs.my_hand, CARD_SORT_MODE_ID,
							CARD_COLOR_MODE_ONLY_CARD_COLOR);
	  break;
	case EVENT_REIZEN_NUMBER:
	case EVENT_REIZEN_CONFIRM:
	case EVENT_REIZEN_PASSE:
	  printf("event=%s, acting_player=%d, rphase=%s, waiting_teller=%d, "
			 "reizwert=%u, winner=%d",
			 event_name_table[e->type], e->acting_player,
			 reiz_phase_name_table[c->cs.sgs.rs.rphase],
			 c->cs.sgs.rs.waiting_teller, c->cs.sgs.rs.reizwert,
			 c->cs.sgs.rs.winner);
	  break;
	case EVENT_REIZEN_DONE:
	  if (c->cs.ist_alleinspieler)
		printf("You are playing alone");
	  else
		printf("You are playing with %s",
			   c->pls[c->cs.sgs.active_players[c->cs.my_partner]]->name);
	  break;
	case EVENT_PLAY_CARD:
	  card_get_name(&e->card, buf);
	  printf("Card %s played. Cards currently on table:", buf);
	  if (c->cs.sgs.curr_stich.played_cards > 0) {
		print_card_array(c->cs.sgs.curr_stich.cs,
						 c->cs.sgs.curr_stich.played_cards);
		printf("\n");
		print_player_turn(c, PRINT_PLAYER_TURN_SHOW_HAND_MODE_DEFAULT);
	  } else
		print_card_array(c->cs.sgs.last_stich.cs,
						 c->cs.sgs.last_stich.played_cards);
	  break;
	case EVENT_STICH_DONE:
	  if (e->stich_winner == c->cs.my_gupid) {
		printf("You won the Stich! \\o/");
	  } else if (!c->cs.ist_alleinspieler
				 && e->stich_winner
							== c->cs.sgs.active_players[c->cs.my_partner]) {
		printf("Your partner won the Stich! \\o/");
	  } else {
		printf("You lost the Stich. Gid good.");
	  }
	  printf("\n");
	  print_player_turn(c, PRINT_PLAYER_TURN_SHOW_HAND_MODE_DEFAULT);
	  break;
	default:
	  printf("Something (%s) happened", event_name_table[e->type]);
  }
  goto skip;

skip:
  printf("\n> ");
  fflush(stdout);
}

static void
stop_client(client *c) {
  client_acquire_state_lock(c);
  client_prepare_exit(c);
  client_release_state_lock(c);
  exit(EXIT_SUCCESS);
}

void *
handle_console_input(void *v) {
  client *c = v;
  char *line = NULL;
  size_t line_size = 0;

  DEBUG_PRINTF("Started console input thread");

  for (;;) {
	printf("> ");
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
	  printf("Invalid command (error %d): %s", error, line);
	  continue;
	}

	printf("command: %s\n", cmd->command);
	printf("%zu args%s\n", cmd->args_length, cmd->args_length > 0 ? ":" : "");
	for (size_t i = 0; i < cmd->args_length; i++)
	  printf("  %zu: %s\n", i, cmd->args[i]);

	int result = 0;

	// ready
	if (!command_equals(cmd, &result, 1, "ready") && result) {
	  if (command_check_arg_length(cmd, 0, &result) || !result) {
		fprintf(stderr, "Expected exactly 0 args for ready, but got %zu\n",
				cmd->args_length);
	  } else {
		execute_ready(c);
	  }
	}

	// play <card index>
	else if (!command_equals(cmd, &result, 1, "play") && result) {
	  if (command_check_arg_length(cmd, 1, &result) || !result) {
		fprintf(stderr, "Expected exactly 1 arg for play, but got %zu\n",
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
		fprintf(stderr, "Expected exactly 0 args for info, but got %zu\n",
				cmd->args_length);
	  } else {
		execute_print_info(c);
	  }
	}

	// exit
	else if (!command_equals(cmd, &result, 2, "exit", "quit") && result) {
	  if (command_check_arg_length(cmd, 0, &result) || !result) {
		fprintf(stderr, "Expected exactly 0 args for exit, but got %zu\n",
				cmd->args_length);
	  } else {
		stop_client(c);
	  }
	}

	// unknown command
	else {
	  fprintf(stderr, "Unknown command: %s", cmd->command);
	}

	command_free(cmd);
  }

  if (line)
	free(line);

  stop_client(c);

  __builtin_unreachable();
}
