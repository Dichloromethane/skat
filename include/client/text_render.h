#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/fttrigon.h>
#include <glad/glad.h>

typedef struct {
  FT_Library ft_library;
  FT_Face font_face;
  int h;
  GLuint list_base;
} text_state;

void text_render_init(text_state *ts);
void text_render_print(text_state *ts, float x, float y, const char *fmt, ...);