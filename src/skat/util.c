#include "skat/util.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int random_fd;

#ifdef HAS_DEBUG_PRINTF
pthread_mutex_t debug_printf_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

__attribute__((constructor(101))) void
open_random_fd(void) {
  random_fd = open("/dev/urandom", O_RDONLY);
  if (random_fd == -1) {
	perror("Error while accessing '/dev/urandom'");
	exit(EXIT_FAILURE);
  }
}

size_t
util_rand_int(const size_t min, const size_t max) {
  size_t random;
  read(random_fd, &random, sizeof(size_t));

  return (random % (max - min)) + min;
}

size_t
round_to_next_pow2(size_t n) {
  return n <= 1 ? 1 : 1 << (32 - __builtin_clz(n - 1));
}

void
perm(int *a, int size, int mask) {
  int r[size];
  int mes, mem;

  if (size <= 1)
	return;

  mes = 32 - __builtin_clz(size - 1);
  mem = (1 << mes) - 1;
  for (int i = 0; i < size; i++) {
	r[mask & mem] = a[i];
	mask >>= mes;
  }
  memcpy(a, r, size * sizeof(int));
}
