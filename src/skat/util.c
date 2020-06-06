#include "skat/util.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int random_fd;

__attribute__((constructor(101))) void
open_random_fd(void) {
  random_fd = open("/dev/urandom", O_RDONLY);
  if (random_fd == -1) {
	perror("Error while accessing '/dev/urandom'");
	exit(1);
  }
}

int
util_rand_int(const int min, const int max) {
  int random;
  read(random_fd, &random, sizeof(int));

  return (random % (max - min)) + min;
}

unsigned int
round_to_next_pow2(unsigned int n) {
  unsigned int p2 = 1;
  while (p2 < n)
	p2 <<= 1u;
  return p2;
}

float
minf(float a, float b) {
  return a < b ? a : b;
}

float
maxf(float a, float b) {
  return a > b ? a : b;
}
