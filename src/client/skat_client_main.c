#include "skat/client.h"
#include "conf.h"
#include <errno.h>
#include <limits.h>
#include <stdint.h>
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
		fprintf(stderr, "Usage: %s [-p port]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
  }

  printf("port=%ld; optind=%d\n", port, optind);

  exit(EXIT_SUCCESS);
}
