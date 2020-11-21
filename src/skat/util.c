#include "skat/util.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef HAS_DEBUG_PRINTF
pthread_mutex_t debug_printf_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

static int
get_random_fd() {
  static int random_fd = -1;

  if (random_fd == -1) {
	DEBUG_PRINTF("Opening /dev/urandom");
	random_fd = open("/dev/urandom", O_RDONLY);
	if (random_fd == -1) {
	  perror("Error while accessing '/dev/urandom'");
	  exit(EXIT_FAILURE);
	}
  }

  return random_fd;
}

size_t
util_rand_int(const size_t min, const size_t max) {
  size_t random;
  read(get_random_fd(), &random, sizeof(size_t));

  return (random % (max - min)) + min;
}

size_t
round_to_next_pow2(size_t n) {
  return n <= 1 ? 1 : 1u << (32u - __builtin_clz(n - 1));
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

int
thread_get_name(pthread_t t, char *name_buffer) {
  return pthread_getname_np(t, name_buffer, THREAD_NAME_SIZE);
}

int
thread_get_name_self(char *name_buffer) {
  return thread_get_name(pthread_self(), name_buffer);
}

int
thread_set_name_va(pthread_t t, const char *name_fmt, va_list ap) {
  char name[THREAD_NAME_SIZE];
  int error = vsnprintf(name, THREAD_NAME_SIZE, name_fmt, ap);
  if (error < 0)
	return 1;
  else if (error >= THREAD_NAME_SIZE)
	return 2;

  error = pthread_setname_np(t, name);
  return error ? 3 : 0;
}

int
thread_set_name(pthread_t t, const char *name_fmt, ...) {
  va_list ap;
  va_start(ap, name_fmt);
  int error = thread_set_name_va(t, name_fmt, ap);
  va_end(ap);
  return error;
}

int
thread_set_name_self(const char *name_fmt, ...) {
  va_list ap;
  va_start(ap, name_fmt);
  int error = thread_set_name_va(pthread_self(), name_fmt, ap);
  va_end(ap);
  return error;
}
