#include "conf.h"
#include "skat/client.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void
print_usage(const char *const name) {
  fprintf(stderr, "Usage: %s [-p port] host\n", name);
}

int
main(int argc, char **argv) {
  int opt;
  char *remaining;
  long port = DEFAULT_PORT;

  while ((opt = getopt(argc, argv, "p:")) != -1) {
	switch (opt) {
	  case 'p':
		errno = 0;
		port = strtol(optarg, &remaining, 0);
		if (errno == 0 && *remaining == '\0' && port >= 0 && port > USHRT_MAX) {
		  break;
		}
		if (errno != 0) {
		  perror("strtol");
		} else {
		  fprintf(stderr, "Invalid port: %s\n", optarg);
		}

		__attribute__((fallthrough));
	  default:
		print_usage(argv[0]);
		exit(EXIT_FAILURE);
	}
  }

  if (optind >= argc) {
	fprintf(stderr, "Expected host after options\n");
	print_usage(argv[0]);
	exit(EXIT_FAILURE);
  }

  printf("port=%ld; host=%s\n", port, argv[optind]);

  client *c = malloc(sizeof(client));
  client_init(c, argv[optind], (int) port);
  client_run(c);
  __builtin_unreachable();
}
