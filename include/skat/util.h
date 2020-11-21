#pragma once

#include "conf.h"
#include <assert.h>
#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a > b ? a : b)

size_t util_rand_int(size_t min, size_t max);
size_t round_to_next_pow2(size_t n);
void perm(int *, int, int);

#define THREAD_NAME_SIZE (16)

int thread_get_name(pthread_t t, char *name_buffer);
int thread_get_name_self(char *name_buffer);
int thread_set_name_va(pthread_t t, const char *name_fmt, va_list ap);
int thread_set_name(pthread_t t, const char *name_fmt, ...);
int thread_set_name_self(const char *name_fmt, ...);

#define COLOR_CLEAR "\e[0m"

#define ERROR_COLOR "\e[31m"
#define ERROR_C(x)  ERROR_COLOR x COLOR_CLEAR

#define TODO_COLOR "\e[33m"
#define TODO_C(x)  TODO_COLOR x COLOR_CLEAR

#define DERROR_PRINTF(fmt, ...) \
  DEBUG_PRINTF_LABEL(ERROR_C("ERROR"), fmt, ##__VA_ARGS__)
#define DTODO_PRINTF(fmt, ...) \
  DEBUG_PRINTF_LABEL(TODO_C("TODO"), fmt, ##__VA_ARGS__)
#define DEBUG_PRINTF(fmt, ...) DEBUG_PRINTF_LABEL("DEBUG", fmt, ##__VA_ARGS__)

#define DPRINTF_COND(cond, ...) \
  do { \
	if (cond) \
	  DEBUG_PRINTF(__VA_ARGS__); \
  } while (0)

#if defined(HAS_DEBUG_PRINTF) && HAS_DEBUG_PRINTF
extern pthread_mutex_t debug_printf_lock;

#define DEBUG_PRINTF_RAW(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#define DEBUG_PRINTF_LABEL(label, fmt, ...) \
  do { \
	pthread_mutex_lock(&debug_printf_lock); \
	DEBUG_PRINTF_RAW(label " (%s:%d)\n     " fmt "\n", __func__, __LINE__, \
					 ##__VA_ARGS__); \
	pthread_mutex_unlock(&debug_printf_lock); \
  } while (0)

#else
#define DEBUG_PRINTF_RAW(...)
#define DEBUG_PRINTF_LABEL(label, fmt, ...)
#endif

#if defined(HAS_DEBUG_LOCK_PRINTF) && HAS_DEBUG_LOCK_PRINTF
#define DEBUG_LOCK 1
#else
#define DEBUG_LOCK 0
#endif

#if defined(HAS_DEBUG_TICK_PRINTF) && HAS_DEBUG_TICK_PRINTF
#define DEBUG_TICK 1
#else
#define DEBUG_TICK 0
#endif

#if defined(HAS_DEBUG_PACKAGE_PRINTF) && HAS_DEBUG_PACKAGE_PRINTF
#define DEBUG_PACKAGE 1
#else
#define DEBUG_PACKAGE 0
#endif

#define ERRNO_CHECK_STR(stmt, str) \
  do { \
	if (stmt) \
	  perror(ERROR_C("ERROR ") str); \
  } while (0)
#define ERRNO_CHECK(stmt) ERRNO_CHECK_STR(stmt, #stmt)
