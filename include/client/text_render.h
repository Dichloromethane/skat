#pragma once

#include "client/shader.h"
#include <stdint.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glad/glad.h>

typedef struct {
  float adv_x;
  float adv_y;
  unsigned int bm_w;
  unsigned int bm_h;
  int bm_l;
  int bm_t;
  unsigned int row_width;
  unsigned int row_height;
  unsigned int tex_x;
  unsigned int tex_y;
  float tex_w;
  float tex_h;
  float off_x;
  float off_y;
} character_data;

typedef struct {
  GLuint texture_id;
  unsigned int width;
  unsigned int height;
  unsigned int max_row_height;
  shader *shd;
  GLint attribute_coord;
  GLint uniform_tex;
#ifndef RAINBOW_TEXT
  GLint uniform_color;
#endif
  GLint uniform_projection;
  GLint uniform_model;
  GLuint vbo;
  character_data char_data[128];
} text_state;

typedef enum {
  TRL_BOTTOM_LEFT,
  TRL_TOP_LEFT,
  TRL_CENTER_LEFT,
  TRL_CENTER
} text_render_loc;

typedef union {
  uint32_t argb;
  struct __attribute__((packed, aligned(4))) {
	uint8_t b;
	uint8_t g;
	uint8_t r;
	uint8_t a;
  };
} color;

static const color BLACK = {.argb = 0xFF000000};
static const color WHITE = {.argb = 0xFFFFFFFF};
static const color RED = {.argb = 0xFFFF0000};
static const color GREEN = {.argb = 0xFF00FF00};
static const color BLUE = {.argb = 0xFF0000FF};

void text_render_init(void);
void text_render_rescale(float width, float height);
void text_render_print(text_render_loc trl, color col, float x, float y,
					   float size, const char *fmt, ...);
void text_render_debug(float x, float y, float s);