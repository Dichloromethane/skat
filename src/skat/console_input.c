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

  stich *stich = &c->cs.sgs.curr_stich;
  card_collection *hand = &c->cs.my_hand;
  card_collection *won_stiche = &c->cs.my_stiche;

  printf("You are %s (gupid=%d, active_player=%d)\n",
		 c->pls[c->cs.my_index]->name.name, c->cs.my_index,
		 c->cs.my_active_player_index);

  printf("Game Type: %d, Trumpf: %d\n", c->cs.sgs.gr.type, c->cs.sgs.gr.trumpf);

  if (c->cs.ist_alleinspieler) {
	printf("You are playing alone, the skat was: ");
	for (int i = 0; i < 2; i++) {
	  card_get_name(&c->cs.skat[i], a);
	  printf("%s ", a);
	}
	printf("\n");
  } else {
	printf("You are playing with %s\n",
		   c->pls[c->cs.sgs.active_players[c->cs.my_partner]]->name.name);
  }

  printf("Current Stich (Vorhand=%s): ",
		 c->pls[c->cs.sgs.active_players[stich->vorhand]]->name.name);
  for (int i = 0; i < stich->played_cards; i++) {
	card_get_name(&stich->cs[i], a);
	printf("%s ", a);
  }
  printf("\n");

  printf("Your hand (%#x): ", *hand);
  for (card_id cur = 0; cur < 32; cur++) {
	card_collection_contains(hand, &cur, &result);
	if (result) {
	  card_get_name(&cur, a);
	  printf("%s ", a);
	}
  }
  printf("\n");

  printf("Your stiche (%#x): ", *won_stiche);
  for (card_id cur = 0; cur < 32; cur++) {
	card_collection_contains(won_stiche, &cur, &result);
	if (result) {
	  card_get_name(&cur, a);
	  printf("%s ", a);
	}
  }
  printf("\n");

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
