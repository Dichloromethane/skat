#include "conf.h"
#include "skat/client.h"
#include "skat/player.h"
#include "skat/util.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void
print_usage(const char *const name) {
  printf("Usage: %s [-r] [-g] [-f] [-h host] [-p port] name\n", name);
}

int start_GRAPHICAL(int fullscreen);

int
main(int argc, char **argv) {
  int opt;
  char *remaining;
  char *host = DEFAULT_HOST;
  long port = DEFAULT_PORT;
  int resume = 0;
  int graphical = 0;
  int fullscreen = 0;

  while ((opt = getopt(argc, argv, "h:p:rgf")) != -1) {
	switch (opt) {
	  case 'r':
		resume = 1;
		break;
	  case 'g':
		graphical = 1;
		break;
	  case 'f':
		fullscreen = 1;
		break;
	  case 'h':
		host = argv[optind - 1];
		break;
	  case 'p':
		errno = 0;
		port = strtol(optarg, &remaining, 0);
		if (errno == 0 && *remaining == '\0' && port >= 0 && port < USHRT_MAX) {
		  break;
		}
		printf("Invalid port: %s\n", optarg);
		if (errno != 0) {
		  printf("strol: %s\n", strerror(errno));
		}

		__attribute__((fallthrough));
	  default:
		print_usage(argv[0]);
		exit(EXIT_FAILURE);
	}
  }

  if (optind >= argc) {
	printf("Expected name after options\n");
	print_usage(argv[0]);
	exit(EXIT_FAILURE);
  }

  char *name = argv[optind];
  if (strlen(name) + 1 >= PLAYER_MAX_NAME_LENGTH) {
	printf("Name was too long\n");
	print_usage(argv[0]);
	exit(EXIT_FAILURE);
  }

  printf("Options: host=%s; port=%ld; name=%s; resume=%d; graphical=%d\n", host,
		 port, name, resume, graphical);

  if (graphical) {
	// TODO: add graphical loop for render and skat logic
	DTODO_PRINTF("TODO: add graphical loop for render and skat logic");
	start_GRAPHICAL(fullscreen);
	exit(EXIT_SUCCESS);
  }

  client *c = malloc(sizeof(client));
  client_init(c, host, (int) port, name);
  client_run(c, resume);
  __builtin_unreachable();
}
