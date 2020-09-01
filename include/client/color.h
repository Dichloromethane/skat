#pragma once

#include <stdint.h>

typedef union {
  uint32_t argb;
  struct __attribute__((packed, aligned(4))) {
	uint8_t b;
	uint8_t g;
	uint8_t r;
	uint8_t a;
  };
} color;

void color_to_rgba_f(color col, float *rgba);
void color_to_rgb_f(color col, float *rgb);

static const color BLACK = {.argb = 0xFF000000};
static const color WHITE = {.argb = 0xFFFFFFFF};
static const color GRAY = {.argb = 0xFF404040};
static const color RED = {.argb = 0xFFFF0000};
static const color GREEN = {.argb = 0xFF00FF00};
static const color BLUE = {.argb = 0xFF0000FF};
static const color YELLOW = {.argb = 0xFFFFFF00};
static const color MAGENTA = {.argb = 0xFFFF00FF};
static const color CYAN = {.argb = 0xFF00FFFF};
