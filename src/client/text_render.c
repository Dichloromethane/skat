#include "client/text_render.h"
#include "client/constants.h"
#include "client/linmath.h"
#include "client/vertex.h"
#include "skat/util.h"
#include <skat/utf8.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static const char *
get_ft_error_message(FT_Error err) {
#undef __FTERRORS_H__
#define FT_ERRORDEF(e, v, s) \
  case e: \
	return s;
#define FT_ERROR_START_LIST switch (err) {
#define FT_ERROR_END_LIST   }
#include FT_ERRORS_H
  return "(Unknown error)";
}

static text_state ts;

void
text_render_init() {
#ifdef RAINBOW_TEXT
  ts.shd = shader_create_load_file("./shader/rainbow_text");
#else
  ts.shd = shader_create_load_file("./shader/text");
#endif
  shader_use(ts.shd);

  ts.attribute_coord = shader_get_attrib_location(ts.shd, "coord");
  ts.uniform_tex = shader_get_uniform_location(ts.shd, "tex");
#ifndef RAINBOW_TEXT
  ts.uniform_color = shader_get_uniform_location(ts.shd, "color");
#endif
  ts.uniform_projection = shader_get_uniform_location(ts.shd, "projection");
  ts.uniform_model = shader_get_uniform_location(ts.shd, "model");

  shader_use(ts.shd);
  mat4x4 projection;
  mat4x4_identity(projection);
  mat4x4_ortho(projection, 0, WIDTH, HEIGHT, 0, -1, 1);
  glUniformMatrix4fv(ts.uniform_projection, 1, GL_FALSE,
					 (const GLfloat *) projection);

  glGenBuffers(1, &ts.vbo);

  FT_Library ft_library;
  FT_Error err = FT_Init_FreeType(&ft_library);
  if (err != FT_Err_Ok) {
	const char *message = get_ft_error_message(err);
	printf("Error: Could not init FreeType: %s (0x%02x)\n", message, err);
	FT_Done_FreeType(ft_library);
	exit(EXIT_FAILURE);
  }

  FT_Face font_face;
  // FIXME: calculate path
  err = FT_New_Face(ft_library, "./font/freefont/FreeMono.otf", 0, &font_face);
  if (err != FT_Err_Ok) {
	const char *message = get_ft_error_message(err);
	printf("Error: Could not load font file: %s (0x%02x)\n", message, err);
	FT_Done_FreeType(ft_library);
	exit(EXIT_FAILURE);
  }

  // err = FT_Set_Pixel_Sizes(font_face, 0, 256);
  err = FT_Set_Char_Size(font_face, 0, 16 * 64, 300, 300);
  if (err != FT_Err_Ok) {
	const char *message = get_ft_error_message(err);
	printf("Error: Could not set pixel sizes: %s (0x%02x)\n", message, err);
	FT_Done_FreeType(ft_library);
	exit(EXIT_FAILURE);
  }

  FT_GlyphSlot g = font_face->glyph;

  ts.width = FONT_TEXTURE_ATLAS_WIDTH;
  ts.height = FONT_TEXTURE_ATLAS_HEIGHT;

  // generate texture
  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &ts.texture_id);
  glBindTexture(GL_TEXTURE_2D, ts.texture_id);
  glUniform1i(ts.uniform_tex, 0);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, ts.width, ts.height, 0, GL_RED,
			   GL_UNSIGNED_BYTE, NULL);

  /* We require 1 byte alignment when uploading texture data */
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  /* Clamping to edges is important to prevent artifacts when scaling */
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  /* Linear filtering usually looks best for text */
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  ts.max_row_height = 0;
  ts.max_glyph_bottom_height = 0;
  ts.max_glyph_top_height = 0;
  ts.max_glyph_height = 0;

  unsigned int height = 0;
  unsigned int row_width = 0;
  unsigned int row_height = 0;
  FT_ULong row_start = 0;

  for (FT_ULong c = 0; c < CHARACTER_COUNT; c++) {
	err = FT_Load_Char(font_face, c, FT_LOAD_RENDER);
	if (err != FT_Err_Ok) {
	  const char *message = get_ft_error_message(err);
	  printf("ERROR: Failed to load glyph: %s (0x%02x)\n", message, err);
	  continue;
	}

	character_data *cd = &ts.char_data[c];
	cd->adv_x = (float) g->advance.x / 64.0f;
	cd->adv_y = (float) g->advance.y / 64.0f;
	cd->bm_w = g->bitmap.width;
	cd->bm_h = g->bitmap.rows;
	cd->bm_l = g->bitmap_left;
	cd->bm_t = g->bitmap_top;
	cd->row_width = 0;
	cd->row_height = 0;
	cd->tex_x = 0;
	cd->tex_y = 0;
	cd->tex_w = (float) g->bitmap.width / (float) ts.width;
	cd->tex_h = (float) g->bitmap.rows / (float) ts.height;
	cd->off_x = 0;
	cd->off_y = 0;

	unsigned int w = cd->bm_w + 1;
	unsigned int h = cd->bm_h + 1;

	if (w >= ts.width || height >= ts.height) {
	  printf("ERROR: font atlas too small\n");
	  FT_Done_FreeType(ft_library);
	  exit(EXIT_FAILURE);
	}

	if (row_width + w >= ts.width) {
	  /*
	  printf("Row full for %d: row_width=%d; row_height=%d; height=%d\n", c,
			 row_width, row_height, height);
	  printf("Retroactively setting width/height/y/y of %d-%d\n", row_start, c);
	   */
	  for (FT_ULong rc = row_start; rc < c; rc++) {
		character_data *rcd = &ts.char_data[rc];
		rcd->row_width = row_width;
		rcd->row_height = row_height;
		rcd->tex_y = height;
		rcd->off_y = (float) height / (float) ts.height;
	  }
	  height += row_height;
	  if (row_height > ts.max_row_height)
		ts.max_row_height = row_height;

	  row_width = 0;
	  row_height = 0;
	  row_start = c;
	}

	cd->tex_x = row_width;
	cd->off_x = (float) row_width / (float) ts.width;

	cd->bm_bottom_h = MAX(0, (int) cd->bm_h - cd->bm_t);
	if (ts.max_glyph_bottom_height < cd->bm_bottom_h)
	  ts.max_glyph_bottom_height = cd->bm_bottom_h;
	if (cd->bm_t > 0 && ts.max_glyph_top_height < (unsigned int) cd->bm_t)
	  ts.max_glyph_top_height = cd->bm_t;

	row_width += w;
	if (h > row_height) {
	  row_height = h;
	}
  }
  /*
  printf("Retroactively setting width/height/y/y of %d-%d\n", row_start,
  CHARACTER_COUNT);
   */
  for (FT_ULong rc = row_start; rc < CHARACTER_COUNT; rc++) {
	character_data *rcd = &ts.char_data[rc];
	rcd->row_width = row_width;
	rcd->row_height = row_height;
	rcd->tex_y = height;
	rcd->off_y = (float) height / (float) ts.height;
  }
  if (row_height > ts.max_row_height) {
	ts.max_row_height = row_height;
  }

  ts.max_glyph_height = ts.max_glyph_bottom_height + ts.max_glyph_top_height;

  /*for (unsigned char c = 0; c < CHARACTER_COUNT; c++) {
	character_data *cd = &ts.char_data[c];
	printf("%3d: %d,%d\n", c, cd->tex_x, cd->tex_y);
  }*/

  for (FT_ULong c = 0; c < CHARACTER_COUNT; c++) {
	err = FT_Load_Char(font_face, c, FT_LOAD_RENDER);
	if (err != FT_Err_Ok) {
	  const char *message = get_ft_error_message(err);
	  printf("ERROR: Failed to load glyph: %s (0x%02x)\n", message, err);
	  continue;
	}

	character_data *cd = &ts.char_data[c];
	glTexSubImage2D(GL_TEXTURE_2D, 0, cd->tex_x, cd->tex_y, cd->bm_w, cd->bm_h,
					GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer);

	/*
	if (ts.max_glyph_top_height == cd->bm_t) {
	  printf("max top height: %d\n", c);
	}
	if (ts.max_glyph_bottom_height == cd->bm_bottom_h) {
	  printf("max bottom height: %d\n", c);
	}
	*/
  }

  FT_Done_FreeType(ft_library);
}

float
text_render_string_width(float size, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  char *text;
  vasprintf(&text, fmt, ap);
  va_end(ap);

  float curX = 0;
  unicode_codepoint_t cp;
  for (const char *next = text; *next;) {
    next = utf8_codepoint(next, &cp);
    character_data *c = &ts.char_data[cp];
	curX += c->adv_x;
  }

  free(text);

  return size * curX;
}

void
text_render_printf(text_render_loc trl, color col, float x, float y, float size,
				   const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  char *text;
  vasprintf(&text, fmt, ap);
  va_end(ap);

  shader_use(ts.shd);
  glBindTexture(GL_TEXTURE_2D, ts.texture_id);
  glUniform1i(ts.uniform_tex, 0);

  mat4x4 model;
  mat4x4_identity(model);
  mat4x4_translate_in_place(model, x, y, 0);
  mat4x4_scale_aniso(model, model, size, size, 1);
  glUniformMatrix4fv(ts.uniform_model, 1, GL_FALSE, (const GLfloat *) model);

  glEnableVertexAttribArray(ts.attribute_coord);
  glBindBuffer(GL_ARRAY_BUFFER, ts.vbo);
  glVertexAttribPointer(ts.attribute_coord, 4, GL_FLOAT, GL_FALSE, 0, 0);

#ifndef RAINBOW_TEXT
  GLfloat rgba[4];
  color_to_rgba_f(col, rgba);
  glUniform4fv(ts.uniform_color, 1, rgba);
#endif

  float offX, offY;
  switch (trl) {
	case TRL_BOTTOM_LEFT:
	  offX = 0.0f;
	  offY = -(float) ts.max_glyph_bottom_height;
	  break;
	case TRL_TOP_LEFT:
	  offX = 0.0f;
	  offY = (float) ts.max_glyph_top_height;
	  break;
	case TRL_CENTER_LEFT:
	  offX = 0.0f;
	  offY = (float) ts.max_glyph_top_height
			 - (float) ts.max_glyph_height / 2.0f;
	  break;
	case TRL_CENTER:
	  offX = -text_render_string_width(1.0f, fmt, ap) / 2.0f;
	  offY = (float) ts.max_glyph_top_height
			 - (float) ts.max_glyph_height / 2.0f;
	  break;
	default:
	  __builtin_unreachable();
  }

  size_t coords_len = 6 * strlen(text);
  vertex2f_st *coords = malloc(coords_len * sizeof(vertex2f_st));
  unsigned int idx = 0;

  float curX = 0, curY = 0;
  unicode_codepoint_t cp;
  for (const char *next = text; *next;) {
    next = utf8_codepoint(next, &cp);
	character_data *c = &ts.char_data[cp];

	float x2 = curX + (float) c->bm_l + offX;
	float y2 = curY - (float) c->bm_t + offY;
	float w = (float) c->bm_w;
	float h = (float) c->bm_h;

	curX += c->adv_x;
	curY += c->adv_y;

	if (!w || !h) {
	  // printf("Skipping char '%c'\n", *p);
	  continue;
	}

	coords[idx++] = (vertex2f_st){x2, y2, c->off_x, c->off_y};
	coords[idx++] = (vertex2f_st){x2, y2 + h, c->off_x, c->off_y + c->tex_h};
	coords[idx++] = (vertex2f_st){x2 + w, y2 + h, c->off_x + c->tex_w,
								  c->off_y + c->tex_h};
	coords[idx++] = (vertex2f_st){x2 + w, y2 + h, c->off_x + c->tex_w,
								  c->off_y + c->tex_h};
	coords[idx++] = (vertex2f_st){x2 + w, y2, c->off_x + c->tex_w, c->off_y};
	coords[idx++] = (vertex2f_st){x2, y2, c->off_x, c->off_y};
  }

  glBufferData(GL_ARRAY_BUFFER, coords_len * sizeof(vertex2f_st), coords,
			   GL_DYNAMIC_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, idx);

  free(coords);
  free(text);
}

void
text_render_debug(float x, float y, float size) {
  float w = (float) ts.width;
  float h = (float) ts.height;

  shader_use(ts.shd);
  glBindTexture(GL_TEXTURE_2D, ts.texture_id);
  glUniform1i(ts.uniform_tex, 0);

#ifndef RAINBOW_TEXT
  GLfloat rgba[4];
  color_to_rgba_f(BLACK, rgba);
  glUniform4fv(ts.uniform_color, 1, rgba);
#endif

  mat4x4 model;
  mat4x4_identity(model);
  mat4x4_translate_in_place(model, x, y, 0);
  mat4x4_scale_aniso(model, model, size, size, 1);
  glUniformMatrix4fv(ts.uniform_model, 1, GL_FALSE, (const GLfloat *) model);

  glEnableVertexAttribArray(ts.attribute_coord);
  glBindBuffer(GL_ARRAY_BUFFER, ts.vbo);
  glVertexAttribPointer(ts.attribute_coord, 4, GL_FLOAT, GL_FALSE, 0, 0);

  vertex2f_st coords[6];
  unsigned int idx = 0;
  coords[idx++] = (vertex2f_st){0, 0, 0, 0};
  coords[idx++] = (vertex2f_st){0, h, 0, 1};
  coords[idx++] = (vertex2f_st){w, h, 1, 1};
  coords[idx++] = (vertex2f_st){w, h, 1, 1};
  coords[idx++] = (vertex2f_st){w, 0, 1, 0};
  coords[idx++] = (vertex2f_st){0, 0, 0, 0};

  glBufferData(GL_ARRAY_BUFFER, sizeof(coords), coords, GL_DYNAMIC_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, idx);
}
