#include "client/text_render.h"

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

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
  FT_Error err = FT_Init_FreeType(&ts->ft_library);
  if (err != FT_Err_Ok) {
	const char *message = get_ft_error_message(err);
	printf("Error: Could not init FreeType Library: %s (%d)\n", message, err);
  }

  // FIXME: calculate path
  err = FT_New_Face(ts->ft_library, "./font/LiberationSerif-Regular.ttf", 0,
					&ts->font_face);
  if (err != FT_Err_Ok) {
	const char *message = get_ft_error_message(err);
	printf("Error: Could not load font file: %s (%d)\n", message, err);
  }

  int h = 16;

  // For Some Twisted Reason, FreeType Measures Font Size
  // In Terms Of 1/64ths Of Pixels.  Thus, To Make A Font
  // h Pixels High, We Need To Request A Size Of h*64.
  // (h << 6 Is Just A Prettier Way Of Writing h*64)
  FT_Set_Char_Size(ts->font_face, h << 6, h << 6, 96, 96);

  // Here We Ask OpenGL To Allocate Resources For
  // All The Textures And Display Lists Which We
  // Are About To Create.
  GLuint textures[128];
  GLuint list_base = glGenLists(128);
  glGenTextures(128, textures);

  // This Is Where We Actually Create Each Of The Fonts Display Lists.
  //for (unsigned char i = 0; i < 128; i++)
	//make_dlist(ts->font_face, i, list_base, textures);

  // We Don't Need The Face Information Now That The Display
  // Lists Have Been Created, So We Free The Associated Resources.
  // FT_Done_Face(ts->font_face);

  // Ditto For The Font Library.
  // FT_Done_FreeType(ts->ft_library);
}
