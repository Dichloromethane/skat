#pragma once

#include<stdio.h>
#include"conf.h"

int util_rand_int(int min, int max);

#ifdef HAS_DEBUG_PRINTF
#define DEBUG_PRINTF(fmt, ...) DEBUG_PRINTF_RAW("DEBUG: " fmt "\n", ## __VA_ARGS__)
#define DEBUG_PRINTF_RAW(fmt, ...) dprintf(2, fmt, ## __VA_ARGS__)
#endif
