#pragma once

#include "skat/client.h"
#include "skat/event.h"

typedef enum print_player_turn_show_hand_mode {
  PRINT_PLAYER_TURN_SHOW_HAND_MODE_ALWAYS,
  PRINT_PLAYER_TURN_SHOW_HAND_MODE_DEFAULT,
  PRINT_PLAYER_TURN_SHOW_HAND_MODE_NEVER
} print_player_turn_show_hand_mode;

typedef enum reizen_input_type {
  REIZEN_INVALID = 0,
  REIZEN_CONFIRM,
  REIZEN_PASSE,
  REIZEN_NEXT,
  REIZEN_VALUE_BASE
} reizen_input_type;

typedef struct exec_reizen_wrapper_args {
  client *c;
  reizen_input_type rit;
} exec_reizen_wrapper_args;

typedef struct client_reizen_callback_args {
  client_action_callback_hdr hdr;
} client_reizen_callback_args;

typedef enum skat_input_type {
  SKAT_INVALID = 0,
  SKAT_TAKE,
  SKAT_LEAVE,
  SKAT_PRESS
} skat_input_type;

typedef struct exec_skat_wrapper_args {
  client *c;
  skat_input_type sit;
  card_id cid1;
  card_id cid2;
} exec_skat_wrapper_args;

typedef struct client_skat_callback_args {
  client_action_callback_hdr hdr;
} client_skat_callback_args;

typedef struct {
  client_action_callback_hdr hdr;
} client_ready_callback_args;

struct client_set_gamerules_args {
  client *c;
  game_rules gr;
};

typedef struct {
  client_action_callback_hdr hdr;
} client_set_gamerules_callback_args;

struct client_play_card_args {
  client *c;
  card_id cid;
};

typedef struct {
  client_action_callback_hdr hdr;
} client_play_card_callback_args;

void *handle_console_input(void *v);
void io_handle_event(client *, event *);
