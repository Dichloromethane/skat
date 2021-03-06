#ifndef REIZEN_HDR
#define REIZEN_HDR

#include "skat/game_rules.h"
#include <stdbool.h>
#include <stdint.h>

#ifndef STRINGIFY
#define STRINGIFY_   #x
#define STRINGIFY(x) STRINGIFY_(x)
#endif

// clang-format off
#ifdef REIZ_PHASE_HDR_TO_STRING
#undef REIZ_PHASE_HDR_TABLE_BEGIN
  #undef FIRST_REIZ_PHASE
  #undef REIZ_PHASE
  #undef REIZ_PHASE_HDR_TABLE_END
  #define REIZ_PHASE_HDR_TABLE_BEGIN char *reiz_phase_name_table[] = {
  #define FIRST_REIZ_PHASE(x) REIZ_PHASE(x)
  #define REIZ_PHASE(x) [REIZ_PHASE_ ## x] = "REIZ_PHASE_" #x
  #define REIZ_PHASE_HDR_TABLE_END , (void *)0};
#else
#define REIZ_PHASE_HDR_TABLE_BEGIN typedef enum {
#define FIRST_REIZ_PHASE(x) REIZ_PHASE_ ## x = 0
#define REIZ_PHASE(x) REIZ_PHASE_ ## x
#define REIZ_PHASE_HDR_TABLE_END } reiz_phase;
#endif

REIZ_PHASE_HDR_TABLE_BEGIN
  FIRST_REIZ_PHASE(INVALID),
  REIZ_PHASE(MITTELHAND_TO_VORHAND),
  REIZ_PHASE(HINTERHAND_TO_WINNER),
  REIZ_PHASE(WINNER),
  REIZ_PHASE(DONE)
REIZ_PHASE_HDR_TABLE_END

#ifndef REIZ_PHASE_HDR_TO_STRING
; // to make clang-format happy
// clang-format on

extern char *reiz_phase_name_table[];

typedef struct reiz_state {
  reiz_phase rphase;
  bool waiting_teller;
  uint16_t reizwert;
  int8_t winner;// ap
} reiz_state;

struct skat_server_state;
typedef struct skat_server_state skat_server_state;

uint16_t reizen_get_next_reizwert(reiz_state *rs);
uint16_t reizen_get_game_value(skat_server_state *cs, int won, int schneider,
							   int schwarz);

int reizen_get_grundwert(game_rules const *gr);

typedef enum loss_type {
  LOSS_TYPE_INVALID = 0,
  LOSS_TYPE_WON,// ERROR: Success
  LOSS_TYPE_WON_DURCHMARSCH,
  LOSS_TYPE_RAMSCH,
  LOSS_TYPE_LOST,
  LOSS_TYPE_LOST_UEBERREIZT
} loss_type;

typedef struct round_result {
  int8_t round_winner;                /* indexed by ap */
  uint8_t player_points[3];           /* indexed by ap */
  uint8_t player_stich_card_count[3]; /* indexed by ap */
  int64_t round_score[3];             /* indexed by ap */
  uint16_t spielwert;
  unsigned loss_type : 3;
  unsigned normal_end : 1;
  unsigned schneider : 1;
  unsigned schwarz : 1;
} round_result;

#endif
#endif
