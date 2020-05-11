#include "conf.h"
#include "skat/server.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
		if (errno == 0 && *remaining == '\0' && port >= 0 && port < USHRT_MAX) {
		  break;
		}
		fprintf(stderr, "Invalid port: %s\n", optarg);
		if (errno != 0) {
		  perror("strtol");
		}

		__attribute__((fallthrough));
	  default:
		fprintf(stderr, "Usage: %s [-p port]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
  }

  printf("port=%ld\n", port);

  server *s = malloc(sizeof(server));
  server_init(s, (int) port);
  server_run(s);
  __builtin_unreachable();
}
