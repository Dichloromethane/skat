#include "client/text_render.h"
#include "client/linmath.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define WIDTH  (640)
#define HEIGHT (480)

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

void
text_render_init(text_state *ts) {
  ts->shd = shader_create_load_file("./shader/text");
  shader_use(ts->shd);

  ts->attribute_coord = shader_get_attrib_location(ts->shd, "coord");
  ts->uniform_tex = shader_get_uniform_location(ts->shd, "tex");
  ts->uniform_color = shader_get_uniform_location(ts->shd, "color");
  ts->uniform_projection = shader_get_uniform_location(ts->shd, "projection");

  mat4x4 projection;
  mat4x4_identity(projection);
  mat4x4_ortho(projection, 0.0f, (float) WIDTH, (float) HEIGHT, 0.0f, -1.0f,
			   1.0f);
  glUniformMatrix4fv(ts->uniform_projection, 1, GL_FALSE,
					 (const GLfloat *) projection);

  glGenBuffers(1, &ts->vbo);

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
  err = FT_New_Face(ft_library, "./font/LiberationSerif-Regular.ttf", 0,
					&font_face);
  if (err != FT_Err_Ok) {
	const char *message = get_ft_error_message(err);
	printf("Error: Could not load font file: %s (0x%02x)\n", message, err);
	FT_Done_FreeType(ft_library);
	exit(EXIT_FAILURE);
  }

  err = FT_Set_Pixel_Sizes(font_face, 0, 64);
  if (err != FT_Err_Ok) {
	const char *message = get_ft_error_message(err);
	printf("Error: Could not set pixel sizes: %s (0x%02x)\n", message, err);
	FT_Done_FreeType(ft_library);
	exit(EXIT_FAILURE);
  }

  FT_GlyphSlot g = font_face->glyph;

  ts->width = 1024;
  ts->height = 1024;

  // generate texture
  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &ts->texture_id);
  glBindTexture(GL_TEXTURE_2D, ts->texture_id);
  glUniform1i(ts->uniform_tex, 0);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, ts->width, ts->height, 0, GL_RED,
			   GL_UNSIGNED_BYTE, NULL);

  /* We require 1 byte alignment when uploading texture data */
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  /* Clamping to edges is important to prevent artifacts when scaling */
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  /* Linear filtering usually looks best for text */
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


  unsigned int height = 0;
  unsigned int row_width = 0;
  unsigned int row_height = 0;
  unsigned char row_start = 0;
  for (unsigned char c = 0; c < 128; c++) {
	err = FT_Load_Char(font_face, c, FT_LOAD_RENDER);
	if (err != FT_Err_Ok) {
	  const char *message = get_ft_error_message(err);
	  printf("ERROR: Failed to load glyph: %s (0x%02x)\n", message, err);
	  continue;
	}

	character_data *cd = &ts->char_data[c];
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
	cd->tex_w = (float) g->bitmap.width / (float) ts->width;
	cd->tex_h = (float) g->bitmap.rows / (float) ts->height;
	cd->off_x = 0;
	cd->off_y = 0;

	unsigned int w = cd->bm_w + 1;
	unsigned int h = cd->bm_h + 1;

	if (w >= ts->width || height >= ts->height) {
	  printf("ERROR: font atlas too small\n");
	  FT_Done_FreeType(ft_library);
	  exit(EXIT_FAILURE);
	}

	if (row_width + w >= ts->width) {
	  printf("Row full for %d: row_width=%d; row_height=%d; height=%d\n", c,
			 row_width, row_height, height);
	  printf("Retroactively setting width/height/y/y of %d-%d\n", row_start, c);
	  for (unsigned rc = row_start; rc < c; rc++) {
		character_data *rcd = &ts->char_data[rc];
		rcd->row_width = row_width;
		rcd->row_height = row_height;
		rcd->tex_y = height;
		rcd->off_y = (float) height / (float) ts->height;
	  }
	  height += row_height;
	  row_width = 0;
	  row_height = 0;
	  row_start = c;
	}

	cd->tex_x = row_width;
	cd->off_x = (float) row_width / (float) ts->width;

	row_width += w;
	if (h > row_height) {
	  row_height = h;
	}
  }
  printf("Retroactively setting width/height/y/y of %d-%d\n", row_start, 128);
  for (unsigned rc = row_start; rc < 128; rc++) {
	character_data *rcd = &ts->char_data[rc];
	rcd->row_width = row_width;
	rcd->row_height = row_height;
	rcd->tex_y = height;
	rcd->off_y = (float) height / (float) ts->height;
  }

  for (unsigned char c = 0; c < 128; c++) {
	character_data *cd = &ts->char_data[c];
	printf("%3d: %dx%d\n", c, cd->tex_x, cd->tex_y);
  }

  for (unsigned char c = 0; c < 128; c++) {
	err = FT_Load_Char(font_face, c, FT_LOAD_RENDER);
	if (err != FT_Err_Ok) {
	  const char *message = get_ft_error_message(err);
	  printf("ERROR: Failed to load glyph: %s (0x%02x)\n", message, err);
	  continue;
	}

	character_data *cd = &ts->char_data[c];
	glTexSubImage2D(GL_TEXTURE_2D, 0, cd->tex_x, cd->tex_y, cd->bm_w, cd->bm_h,
					GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer);
  }

  FT_Done_FreeType(ft_library);
}

// A Fairly Straightforward Function That Pushes
// A Projection Matrix That Will Make Object World
// Coordinates Identical To Window Coordinates.
static void
pushScreenCoordinateMatrix() {
  glPushAttrib(GL_TRANSFORM_BIT);
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(viewport[0], viewport[2], viewport[1], viewport[3], -1, 1);
  glPopAttrib();
}

// Pops The Projection Matrix Without Changing The Current
// MatrixMode.
static void
pop_projection_matrix() {
  glPushAttrib(GL_TRANSFORM_BIT);
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glPopAttrib();
}

typedef struct {
  GLfloat x;
  GLfloat y;
  GLfloat s;
  GLfloat t;
} point;

void
text_render_print(text_state *ts, float x, float y, float sx, float sy,
				  const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int size = vsnprintf(NULL, 0, fmt, ap);
  char *text = malloc(size + 1);
  vsprintf(text, fmt, ap);
  va_end(ap);

  // printf("%s\n", text);

  /*pushScreenCoordinateMatrix();
  glPushAttrib(GL_LIST_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT |
			   GL_TRANSFORM_BIT); glMatrixMode(GL_MODELVIEW);
  glDisable(GL_LIGHTING); glEnable(GL_TEXTURE_2D); glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  float modelview_matrix[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, modelview_matrix);

  shader_use(ts->shd);

  GLfloat transparent_green[] = {0, 1, 0, 0.5};
  glUniform4fv(ts->uniform_color, 1, transparent_green);*/

  shader_use(ts->shd);
  /* Use the texture containing the atlas */
  glBindTexture(GL_TEXTURE_2D, ts->texture_id);
  glUniform1i(ts->uniform_tex, 0);

  /* Set up the VBO for our vertex data */
  glEnableVertexAttribArray(ts->attribute_coord);
  glBindBuffer(GL_ARRAY_BUFFER, ts->vbo);
  glVertexAttribPointer(ts->attribute_coord, 4, GL_FLOAT, GL_FALSE, 0, 0);

  point coords[6 * strlen(text)];
  unsigned int idx = 0;

  /* Loop through all characters */
  for (char *p = text; *p; p++) {
	/* Calculate the vertex and texture coordinates */
	character_data *c = &ts->char_data[*p];
	float x2 = x + (float) c->bm_l * sx;
	float y2 = -y - (float) c->bm_t * sy;
	float w = (float) c->bm_w * sx;
	float h = (float) c->bm_h * sy;

	/* Advance the cursor to the start of the next character */
	x += c->adv_x * sx;
	y += c->adv_y * sy;

	/* Skip glyphs that have no pixels */
	if (!w || !h)
	  continue;

	coords[idx++] = (point){x2, -y2, c->off_x, c->off_y};
	coords[idx++] = (point){x2 + w, -y2, c->off_x + c->tex_w, c->off_y};
	coords[idx++] = (point){x2, -y2 - h, c->off_x, c->off_y + c->tex_h};
	coords[idx++] = (point){x2 + w, -y2, c->off_x + c->tex_w, c->off_y};
	coords[idx++] = (point){x2, -y2 - h, c->off_x, c->off_y + c->tex_h};
	coords[idx++] =
			(point){x2 + w, -y2 - h, c->off_x + c->tex_w, c->off_y + c->tex_h};
  }

  /* Draw all the character on the screen in one go */
  glBufferData(GL_ARRAY_BUFFER, sizeof(coords), coords, GL_DYNAMIC_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, idx);

  glDisableVertexAttribArray(ts->attribute_coord);

  /*glPopAttrib();
  pop_projection_matrix();*/

  free(text);

  /*pushScreenCoordinateMatrix();

  GLuint font = 0;// ts->list_base;
  glPushAttrib(GL_LIST_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT |
  GL_TRANSFORM_BIT); glMatrixMode(GL_MODELVIEW); glDisable(GL_LIGHTING);
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glListBase(font);
  float modelview_matrix[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, modelview_matrix);

  glPushMatrix();
  glLoadIdentity();
  glTranslatef(x, y, 0);
  glMultMatrixf(modelview_matrix);

  size_t len = strlen(text);
  glCallLists(len, GL_UNSIGNED_BYTE, text);

  glPopMatrix();

  glPopAttrib();
  pop_projection_matrix();*/
}

void
text_render_debug(text_state *ts, float x, float y, float sx, float sy) {
  float w = (float) ts->width * sx;
  float h = (float) ts->height * sy;

  // pushScreenCoordinateMatrix();
  shader_use(ts->shd);
  glBindTexture(GL_TEXTURE_2D, ts->texture_id);
  glUniform1i(ts->uniform_tex, 0);

  /* Set up the VBO for our vertex data */
  glEnableVertexAttribArray(ts->attribute_coord);
  glBindBuffer(GL_ARRAY_BUFFER, ts->vbo);
  glVertexAttribPointer(ts->attribute_coord, 4, GL_FLOAT, GL_FALSE, 0, 0);

  point coords[6];
  unsigned int idx = 0;
  coords[idx++] = (point){x, -y, 0, 0};
  coords[idx++] = (point){x + w, -y, 1, 0};
  coords[idx++] = (point){x, -y - h, 0, 1};
  coords[idx++] = (point){x + w, -y, 1, 0};
  coords[idx++] = (point){x, -y - h, 0, 1};
  coords[idx++] = (point){x + w, -y - h, 1, 1};

  /* Draw all the character on the screen in one go */
  glBufferData(GL_ARRAY_BUFFER, sizeof(coords), coords, GL_DYNAMIC_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, idx);

  glDisableVertexAttribArray(ts->attribute_coord);
  // pop_projection_matrix();
}
