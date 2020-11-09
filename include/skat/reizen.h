#ifndef REIZEN_HDR
#define REIZEN_HDR

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
  int waiting_teller;
  uint16_t reizwert;
  int winner;// ap
} reiz_state;

typedef struct skat_server_state skat_server_state;

uint16_t reizen_get_next_reizwert(reiz_state *rs);
uint16_t reizen_get_game_value(skat_server_state *cs, int won, int schneider,
							   int schwarz);

#endif
#endif

// wenn spiel fertig:
// gewinnbedingung überprüfen: 61pkt/0 stiche & schneider/schwarz
// berechne tatsächlichen spielwert
// vergleiche mit reizwert
// wenn überreizt: s.u.

// sonst:

// wenn gewonnen: alleinspieler erhält tatsächlichen spielwert als plus

// wenn verloren: alleinspieler erhält doppelten tatsächlichen spielwert als
// minus

// wenn überreizt: alleinspieler erhält doppelten minimalen möglichen spielwert
// über reizwert in gespieltem spiel als minus

// =======================

// reizen
// 1. mittelhand sagt, vorhand hört
// 2. hinterhand sagt, gewinner von 1 hört
// sagen: reizen <"number" | next | (weg | passe)>
// hören: reizen <(weg | passe) | ja>

// kein gewinner: ramschen

// gewinner = alleinspieler

// skat aufnehmen: ja oder nein
// skat <take | leave>
// nach aufnahme: drücken
// druecken <card id 1> <card id 2>

// spielansage
// spiel <kreuz | pik | herz | karo | grand> [hand] [schneider] [schwarz]
// [ouvert | offen]
// spiel <null> [hand] [ouvert | offen]
