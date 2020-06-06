#include "client/line_render.h"
#include "client/constants.h"
#include "client/linmath.h"
#include "client/shader.h"
#include "client/vertex.h"
#include "skat/util.h"

static struct {
  shader *shd;
  GLuint vbo;
  GLint attribute_pos;
  GLint uniform_tex;
  GLint uniform_color;
  GLint uniform_projection;
  GLint uniform_model;
} ls;

void
line_render_init() {
  ls.shd = shader_create_load_file("./shader/line");
  shader_use(ls.shd);

  ls.attribute_pos = shader_get_attrib_location(ls.shd, "pos");
  ls.uniform_color = shader_get_uniform_location(ls.shd, "color");
  ls.uniform_projection = shader_get_uniform_location(ls.shd, "projection");
  ls.uniform_model = shader_get_uniform_location(ls.shd, "model");

  mat4x4 projection;
  mat4x4_identity(projection);
  mat4x4_ortho(projection, 0, WIDTH, HEIGHT, 0, -1, 1);
  glUniformMatrix4fv(ls.uniform_projection, 1, GL_FALSE,
					 (const GLfloat *) projection);

  glGenBuffers(1, &ls.vbo);
}

void
render_line(color col, float start_x, float start_y, float end_x, float end_y) {
  shader_use(ls.shd);

  GLfloat rgba[4];
  color_to_rgba_f(col, rgba);
  glUniform4fv(ls.uniform_color, 1, rgba);

  mat4x4 model;
  mat4x4_identity(model);
  mat4x4_translate_in_place(model, start_x, start_y, 0);
  glUniformMatrix4fv(ls.uniform_model, 1, GL_FALSE, (const GLfloat *) model);

  glEnableVertexAttribArray(ls.attribute_pos);
  glBindBuffer(GL_ARRAY_BUFFER, ls.vbo);
  glVertexAttribPointer(ls.attribute_pos, 2, GL_FLOAT, GL_FALSE, 0, 0);

  vertex2f coords[2];
  coords[0] = (vertex2f){0, 0};
  coords[1] = (vertex2f){end_x - start_x, end_y - start_y};

  glBufferData(GL_ARRAY_BUFFER, sizeof(coords), coords, GL_DYNAMIC_DRAW);
  glDrawArrays(GL_LINES, 0, 2);
}

void
render_box(color col, float start_x, float start_y, float width, float height) {
  float min_x = minf(start_x, start_x + width);
  float max_x = maxf(start_x, start_x + width);
  float min_y = minf(start_y, start_y + height);
  float max_y = maxf(start_y, start_y + height);

  shader_use(ls.shd);

  GLfloat rgba[4];
  color_to_rgba_f(col, rgba);
  glUniform4fv(ls.uniform_color, 1, rgba);

  mat4x4 model;
  mat4x4_identity(model);
  mat4x4_translate_in_place(model, min_x, min_y, 0);
  glUniformMatrix4fv(ls.uniform_model, 1, GL_FALSE, (const GLfloat *) model);

  glEnableVertexAttribArray(ls.attribute_pos);
  glBindBuffer(GL_ARRAY_BUFFER, ls.vbo);
  glVertexAttribPointer(ls.attribute_pos, 2, GL_FLOAT, GL_FALSE, 0, 0);

  vertex2f coords[5];
  coords[0] = (vertex2f){0, 0};
  coords[1] = (vertex2f){0, max_y - min_y};
  coords[2] = (vertex2f){max_x - min_x, max_y - min_y};
  coords[3] = (vertex2f){max_x - min_x, 0};
  coords[4] = (vertex2f){0, 0};

  glBufferData(GL_ARRAY_BUFFER, sizeof(coords), coords, GL_DYNAMIC_DRAW);
  glDrawArrays(GL_LINE_STRIP, 0, 5);
}
