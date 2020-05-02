#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

typedef struct {
  FT_Library ft_library;
  FT_Face font_face;
} text_state;

void text_render_init(text_state *ts);