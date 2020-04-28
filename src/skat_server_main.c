#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "server.h"

int
main(int argc, char **argv) {
  server *s;
  if (argc != 2) {
	dprintf(2, "usage: %s port\n", argv[0]);
	exit(1);
  }
  s = malloc(sizeof(server));
  server_init(s, atoi(argv[1]));
  server_run(s);
  __builtin_unreachable();
}
