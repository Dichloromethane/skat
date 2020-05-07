#pragma once

#include"conf.h"

int util_rand_int(int min, int max);

#ifdef DEBUG_PRINTF
#define DEBUG_NOTIFY(fmt, ...) DEBUG_PRINTF_RAW("NOTIFY: " fmt)
#define DEBUG_NOTIFY_RAW(fmt, ...) dprintf(2, fmt, __VA_ARGS__)
#endif
