#include "skat/console_input.h"
#include "skat/card.h"
#include "skat/card_collection.h"
#include "skat/client.h"
#include "skat/util.h"
#include <stdio.h>
#include <string.h>

static void
print_info_exec(void *p) {
  client *c = p;

  client_acquire_state_lock(c);

  int result;
  char a[4];

  game_phase phase = c->cs.sgs.cgphase;
  stich *last_stich = &c->cs.sgs.last_stich;
  stich *stich = &c->cs.sgs.curr_stich;
  card_collection *hand = &c->cs.my_hand;
  card_collection *won_stiche = &c->cs.my_stiche;

  printf("You are %s[gupid=%d, active_player=%d]\n",
		 c->pls[c->cs.my_index]->name.name, c->cs.my_index,
		 c->cs.my_active_player_index);

  printf("You are playing with:");
  for (int i = 0; i < 4; i++) {
	player *pl = c->pls[i];
	if (pl == NULL)
	  continue;

	int ap = -1;
	for (int j = 0; j < 3; j++) {
	  if (c->cs.sgs.active_players[j] == pl->index) {
		ap = j;
		break;
	  }
	}
	printf(" %s[gupid=%d, active_player=%d]", pl->name.name, pl->index, ap);
  }
  printf("\n");

  printf("Game Phase: %s, Type: %d, Trumpf: %d\n", game_phase_name_table[phase],
		 c->cs.sgs.gr.type, c->cs.sgs.gr.trumpf);

  if (phase != GAME_PHASE_INVALID && phase != GAME_PHASE_SETUP
	  && phase != GAME_PHASE_BETWEEN_ROUNDS) {
	if (c->cs.ist_alleinspieler) {
	  printf("You are playing alone, the skat was:");
	  for (int i = 0; i < 2; i++) {
		card_get_name(&c->cs.skat[i], a);
		printf(" %s", a);
	  }
	  printf("\n");
	} else {
	  printf("You are playing with %s\n",
			 c->pls[c->cs.sgs.active_players[c->cs.my_partner]]->name.name);
	}

	if (c->cs.sgs.stich_num > 0) {
	  printf("Last Stich (num=%d, vorhand=%s, winner=%s):", c->cs.sgs.stich_num,
			 c->pls[c->cs.sgs.active_players[last_stich->vorhand]]->name.name,
			 c->pls[c->cs.sgs.active_players[last_stich->winner]]->name.name);
	  for (int i = 0; i < last_stich->played_cards; i++) {
		card_get_name(&last_stich->cs[i], a);
		printf(" %s", a);
	  }
	  printf("\n");
	}
	printf("Current Stich (num=%d, vorhand=%s):", c->cs.sgs.stich_num,
		   c->pls[c->cs.sgs.active_players[stich->vorhand]]->name.name);
	for (int i = 0; i < stich->played_cards; i++) {
	  card_get_name(&stich->cs[i], a);
	  printf(" %s", a);
	}
	printf("\n");

	int player_turn =
			c->cs.sgs
					.active_players[(stich->vorhand + stich->played_cards) % 3];
	if (c->cs.my_index == player_turn) {
	  printf("It is YOUR turn\n");
	} else {
	  printf("it is %s's turn\n", c->pls[player_turn]->name.name);
	}

	printf("Your hand (%#x):", *hand);
	for (card_id cur = 0; cur < 32; cur++) {
	  card_collection_contains(hand, &cur, &result);
	  if (result) {
		card_get_name(&cur, a);
		printf(" %s", a);
	  }
	}
	printf("\n");

	printf("Your stiche (%#x):", *won_stiche);
	for (card_id cur = 0; cur < 32; cur++) {
	  card_collection_contains(won_stiche, &cur, &result);
	  if (result) {
		card_get_name(&cur, a);
		printf(" %s", a);
	  }
	}
	printf("\n");
  }

  client_release_state_lock(c);
}

struct client_play_card_args {
  client *c;
  unsigned int card_index;
};

static void
client_play_card_wrapper(void *v) {
  struct client_play_card_args *args = v;
  client_play_card(args->c, args->card_index);
  free(args);
}

static void
execute_print_info(client *c) {
  async_callback acb;

  acb = (async_callback){.do_stuff = print_info_exec, .data = c};

  exec_async(&c->acq, &acb);
}

static void
execute_ready(client *c) {
  async_callback acb;

  // STFU
  acb = (async_callback){.do_stuff = (void (*)(void *)) client_ready,
						 .data = c};

  exec_async(&c->acq, &acb);
}

static void
execute_play_card(client *c, unsigned int card_index) {
  async_callback acb;

  struct client_play_card_args *args =
		  malloc(sizeof(struct client_play_card_args));
  args->c = c;
  args->card_index = card_index;

  acb = (async_callback){.do_stuff = client_play_card_wrapper, .data = args};

  exec_async(&c->acq, &acb);
}

#define MATCH_COMMAND(c, s) if (!strncmp(c, s, sizeof(s)))

// This is an exceedingly safe scanf-based parser
// No judge plz
_Noreturn void *
handle_console_input(void *cc) {
  client *c;
  char *command;
  int card_index;

  DEBUG_PRINTF("Started console input thread");

  c = cc;

  for (;;) {
	// read command (first word)
	printf("Awaiting your command: ");
	scanf("%ms", &command);

	// ready
	MATCH_COMMAND(command, "ready") { execute_ready(c); }

	// play <card index>
	else MATCH_COMMAND(command, "play") {
	  scanf("%d", &card_index);
	  execute_play_card(c, card_index);
	}

	// info
	else MATCH_COMMAND(command, "info") {
	  execute_print_info(c);
	}

	else {
	  printf("Invalid command: %s\n", command);
	}
	// ...


	// enqueue command exec on async
	free(command);
  }
  __builtin_unreachable();
}
