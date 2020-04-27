#include "util.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
util_rand_int(const int min, const int max) {
  int fd = open("/dev/urandom", O_RDONLY);
  if (fd == -1) {
	perror("Error while accessing '/dev/urandom': ");
	exit(1);
  }

  int random;
  read(fd, &random, sizeof(int));
  close(fd);

  return (random % (max - min)) + min;
}
