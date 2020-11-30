#include "client/game_rules_printer.h"
#include <stdio.h>

void
print_game_rules_info(const game_rules *const gr) {
  printf("The game is ");
  switch (gr->type) {
	case GAME_TYPE_RAMSCH:
	  printf("Ramsch");
	  break;
	case GAME_TYPE_NULL:
	  printf("Null");
	  break;
	case GAME_TYPE_GRAND:
	  printf("Grand");
	  break;
	case GAME_TYPE_COLOR:
	  switch (gr->trumpf) {
		case COLOR_KREUZ:
		  printf("Kreuz");
		  break;
		case COLOR_PIK:
		  printf("Pik");
		  break;
		case COLOR_HERZ:
		  printf("Herz");
		  break;
		case COLOR_KARO:
		  printf("Karo");
		  break;
		default:
		  printf("(Invalid card color)");
	  }
	  break;
	default:
	  printf("(Invalid game type)");
  }

  if (gr->hand)
	printf(" Hand");
  if (gr->schneider_angesagt)
	printf(" Schneider");
  if (gr->schwarz_angesagt)
	printf(" Schwarz");
  if (gr->ouvert)
	printf(" Ouvert");

  printf(".\n");
}
