#include "client/text_render.h"
#include "skat/util.h"

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/fttrigon.h>
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

static void
make_dlist(FT_Face face, char ch, GLuint list_base, GLuint *tex_base) {
  // The First Thing We Do Is Get FreeType To Render Our Character
  // Into A Bitmap.  This Actually Requires A Couple Of FreeType Commands:

  // Load The Glyph For Our Character.
  if (FT_Load_Glyph(face, FT_Get_Char_Index(face, ch), FT_LOAD_DEFAULT))
	// throw std::runtime_error("FT_Load_Glyph failed");
	printf("Error: FT_Load_Glyph failed\n");

  // Move The Face's Glyph Into A Glyph Object.
  FT_Glyph glyph;
  if (FT_Get_Glyph(face->glyph, &glyph))
	// throw std::runtime_error("FT_Get_Glyph failed");
	printf("Error: FT_Get_Glyph failed\n");

  // Convert The Glyph To A Bitmap.
  FT_Glyph_To_Bitmap(&glyph, ft_render_mode_normal, 0, 1);
  FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph) glyph;

  // This Reference Will Make Accessing The Bitmap Easier.
  FT_Bitmap bitmap = bitmap_glyph->bitmap;
  // Use Our Helper Function To Get The Widths Of
  // The Bitmap Data That We Will Need In Order To Create
  // Our Texture.
  unsigned int width = round_to_next_pow2(bitmap.width);
  unsigned int height = round_to_next_pow2(bitmap.rows);

  // Allocate Memory For The Texture Data.
  GLubyte *expanded_data = malloc(sizeof(GLubyte) * 2 * width * height);

  // Here We Fill In The Data For The Expanded Bitmap.
  // Notice That We Are Using A Two Channel Bitmap (One For
  // Channel Luminosity And One For Alpha), But We Assign
  // Both Luminosity And Alpha To The Value That We
  // Find In The FreeType Bitmap.
  // We Use The ?: Operator To Say That Value Which We Use
  // Will Be 0 If We Are In The Padding Zone, And Whatever
  // Is The FreeType Bitmap Otherwise.
  for (unsigned int j = 0; j < height; j++) {
	for (unsigned int i = 0; i < width; i++) {
	  expanded_data[2 * (i + j * width)] = 255;
	  expanded_data[2 * (i + j * width) + 1] =
			  (i >= bitmap.width || j >= bitmap.rows)
					  ? 0
					  : bitmap.buffer[i + bitmap.width * j];
	}
  }

  // Now We Just Setup Some Texture Parameters.
  glBindTexture(GL_TEXTURE_2D, tex_base[ch]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  // Here We Actually Create The Texture Itself, Notice
  // That We Are Using GL_LUMINANCE_ALPHA To Indicate That
  // We Are Using 2 Channel Data.
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_LUMINANCE_ALPHA,
			   GL_UNSIGNED_BYTE, expanded_data);

  // With The Texture Created, We Don't Need The Expanded Data Anymore.
  free(expanded_data);
  // Now We Create The Display List
  glNewList(list_base + ch, GL_COMPILE);

  glBindTexture(GL_TEXTURE_2D, tex_base[ch]);

  glPushMatrix();

  // First We Need To Move Over A Little So That
  // The Character Has The Right Amount Of Space
  // Between It And The One Before It.
  glTranslatef(bitmap_glyph->left, 0, 0);

  // Now We Move Down A Little In The Case That The
  // Bitmap Extends Past The Bottom Of The Line
  // This Is Only True For Characters Like 'g' Or 'y'.
  glTranslatef(0, bitmap_glyph->top - bitmap.rows, 0);

  // Now We Need To Account For The Fact That Many Of
  // Our Textures Are Filled With Empty Padding Space.
  // We Figure What Portion Of The Texture Is Used By
  // The Actual Character And Store That Information In
  // The x And y Variables, Then When We Draw The
  // Quad, We Will Only Reference The Parts Of The Texture
  // That Contains The Character Itself.
  float x = (float) bitmap.width / (float) width,
		y = (float) bitmap.rows / (float) height;

  // Here We Draw The Texturemapped Quads.
  // The Bitmap That We Got From FreeType Was Not
  // Oriented Quite Like We Would Like It To Be,
  // But We Link The Texture To The Quad
  // In Such A Way That The Result Will Be Properly Aligned.
  glBegin(GL_QUADS);
  glTexCoord2d(0, 0);
  glVertex2f(0, bitmap.rows);
  glTexCoord2d(0, y);
  glVertex2f(0, 0);
  glTexCoord2d(x, y);
  glVertex2f(bitmap.width, 0);
  glTexCoord2d(x, 0);
  glVertex2f(bitmap.width, bitmap.rows);
  glEnd();
  glPopMatrix();
  glTranslatef(face->glyph->advance.x >> 6, 0, 0);

  // Increment The Raster Position As If We Were A Bitmap Font.
  // (Only Needed If You Want To Calculate Text Length)
  // glBitmap(0,0,0,0,face->glyph->advance.x >> 6,0,NULL);

  // Finish The Display List
  glEndList();
}