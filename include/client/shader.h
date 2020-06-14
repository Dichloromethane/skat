#pragma once

#define _GNU_SOURCE

#include "glad/glad.h"

typedef struct {
  GLuint vertex_shader;
  GLuint fragment_shader;
  GLuint program;
} shader;

shader *shader_create_empty();
shader *shader_create_load_text(const char *vertex_shader_source,
								const char *fragment_shader_source);
shader *shader_create_load_file(const char *shader_name);

void shader_load_text(shader *shdr, const char *shader_source, GLenum type);
void shader_load_file(shader *shdr, const char *shader_name, GLenum type);
void shader_link(shader *shdr);

GLint shader_get_uniform_location(const shader *shdr, const char *name);
GLint shader_get_attrib_location(const shader *shdr, const char *name);

void shader_use(const shader *shdr);

void shader_free(shader *shdr);