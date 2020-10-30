#pragma once

#include <stdint.h>

typedef enum reiz_phase {
  REIZ_PHASE_INVALID = 0,
  REIZ_PHASE_MITTELHAND_TO_VORHAND,
  REIZ_PHASE_HINTERHAND_TO_WINNER,
  REIZ_PHASE_DONE
} reiz_phase;

typedef struct reiz_state {
  reiz_phase rphase;
  int telling_player;  // ap
  int listening_player;// ap
  uint16_t reizwert;
  int winner;// ap
} reiz_state;

typedef struct skat_client_state skat_client_state;

uint16_t reizen_get_game_value(skat_client_state *cs);
