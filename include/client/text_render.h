#pragma once

#include "client/shader.h"

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
  shader *shd;
  GLint attribute_coord;
  GLint uniform_tex;
  GLint uniform_color;
  GLint uniform_projection;
  GLint uniform_model;
  GLuint vbo;
  character_data char_data[128];
} text_state;

void text_render_init(void);
void text_render_rescale(float width, float height);
void text_render_print(float x, float y, float s, const char *fmt, ...);
void text_render_debug(float x, float y, float s);