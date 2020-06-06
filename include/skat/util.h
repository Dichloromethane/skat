#pragma once

#include "conf.h"
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

int util_rand_int(int min, int max);
unsigned int round_to_next_pow2(unsigned int n);
float minf(float a, float b);
float maxf(float a, float b);

#define ERROR_COLOR "\e[31m"
#define COLOR_CLEAR "\e[0m"
#define ERROR_C(x)  ERROR_COLOR x COLOR_CLEAR

#define TODO_COLOR "\e[33m"
#define TODO_C(x) TODO_COLOR x COLOR_CLEAR


#define DERROR_PRINTF(fmt, ...) \
  DEBUG_PRINTF_LABEL(ERROR_C("ERROR"), fmt, ##__VA_ARGS__)
#define DTODO_PRINTF(fmt, ...) \
  DEBUG_PRINTF_LABEL(TODO_C("TODO "), fmt, ##__VA_ARGS__)
#define DEBUG_PRINTF(fmt, ...) DEBUG_PRINTF_LABEL("DEBUG", fmt, ##__VA_ARGS__)

#define DEBUG_PRINTF_LABEL(label, fmt, ...) \
  DEBUG_PRINTF_RAW(label " (%s): " fmt "\n", __func__, ##__VA_ARGS__)

#define DPRINTF_COND(cond, ...) do { \
								  if(cond)\
									DEBUG_PRINTF(__VA_ARGS__);\
								} while(0)

#ifdef HAS_DEBUG_PRINTF
#define DEBUG_PRINTF_RAW(fmt, ...) dprintf(2, fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINTF_RAW(...)
#endif

#ifdef HAS_DEBUG_LOCK_PRINTF
#define DEBUG_LOCK 1
#else
#define DEBUG_LOCK 0
#endif

#ifdef HAS_DEBUG_TICK_PRINTF
#define DEBUG_TICK 1
#else
#define DEBUG_TICK 0
#endif

#define ERRNO_CHECK_STR(stmt, str) \
  do { \
	if (stmt) \
	  perror(ERROR_C("ERROR ") str); \
  } while (0)
#define ERRNO_CHECK(stmt) ERRNO_CHECK_STR(stmt, #stmt)

#ifdef SYS_gettid
// explicit syscall. Nice.
#define gettid() ((pid_t) syscall(SYS_gettid))
#else
#error "SYS_gettid unavailable on this system"
#endif
