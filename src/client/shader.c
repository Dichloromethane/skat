#include "client/shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
print_log(GLuint id) {
  int is_shader;
  if (glIsShader(id)) {
	is_shader = 1;
  } else if (glIsProgram(id)) {
	is_shader = 0;
  } else {
	printf("Id not a valid Shader or Program\n");
	return;
  }

  GLint len;
  if (is_shader) {
	glGetShaderiv(id, GL_INFO_LOG_LENGTH, &len);
  } else {
	glGetProgramiv(id, GL_INFO_LOG_LENGTH, &len);
  }

  GLchar *text = calloc(len, sizeof(GLchar));
  if (is_shader) {
	glGetShaderInfoLog(id, len, &len, text);
	printf("Shader Log:\n%s\n", text);
  } else {
	glGetProgramInfoLog(id, len, &len, text);
	printf("Program Log:\n%s\n", text);
  }
}

shader *
shader_create_empty() {
  shader *shdr = malloc(sizeof(shader));
  shdr->program = glCreateProgram();
  return shdr;
}

shader *
shader_create_load_text(const char *vertex_shader_source,
						const char *fragment_shader_source) {
  shader *shdr = shader_create_empty();
  shader_load_text(shdr, vertex_shader_source, GL_VERTEX_SHADER);
  shader_load_text(shdr, fragment_shader_source, GL_FRAGMENT_SHADER);
  shader_link(shdr);
  return shdr;
}

shader *
shader_create_load_file(const char *shader_name) {
  shader *shdr = shader_create_empty();
  shader_load_file(shdr, shader_name, GL_VERTEX_SHADER);
  shader_load_file(shdr, shader_name, GL_FRAGMENT_SHADER);
  shader_link(shdr);
  return shdr;
}

void
shader_load_text(shader *const shdr, const char *const shader_source,
				 const GLenum type) {
  if (type == GL_VERTEX_SHADER) {
	if (glIsShader(shdr->vertex_shader)) {
	  printf("Vertex Shader already set!\n");
	  exit(EXIT_FAILURE);
	}
  } else if (type == GL_FRAGMENT_SHADER) {
	if (glIsShader(shdr->fragment_shader)) {
	  printf("Fragment Shader already set!\n");
	  exit(EXIT_FAILURE);
	}
  } else {
	printf("Shader Type not implemented!\n");
	exit(EXIT_FAILURE);
  }

  GLuint shader_id = glCreateShader(type);
  glShaderSource(shader_id, 1, &shader_source, NULL);
  glCompileShader(shader_id);

  GLint status;
  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &status);
  if (!status) {
	printf("Error while compiling Shader\n");
	print_log(shader_id);
	shader_free(shdr);
	exit(EXIT_FAILURE);
  }

  glAttachShader(shdr->program, shader_id);

  if (type == GL_VERTEX_SHADER) {
	shdr->vertex_shader = shader_id;
  } else {
	shdr->fragment_shader = shader_id;
  }
}

void
shader_load_file(shader *const shdr, const char *const shader_name,
				 const GLenum type) {
  if (type == GL_VERTEX_SHADER) {
	if (glIsShader(shdr->vertex_shader)) {
	  printf("Vertex Shader already set!\n");
	  exit(EXIT_FAILURE);
	}
  } else if (type == GL_FRAGMENT_SHADER) {
	if (glIsShader(shdr->fragment_shader)) {
	  printf("Fragment Shader already set!\n");
	  exit(EXIT_FAILURE);
	}
  } else {
	printf("Shader Type not implemented!\n");
	exit(EXIT_FAILURE);
  }

  size_t len = strlen(shader_name);
  char filename[len + 3 + 1];
  strcpy(filename, shader_name);
  if (type == GL_VERTEX_SHADER) {
	strcat(filename, ".vs");
  } else {
	strcat(filename, ".fs");
  }

  FILE *f = fopen(filename, "r");
  if (f == NULL) {
	perror("Shader File could not be opened");
  }

  size_t shader_source_size = BUFSIZ;
  char *shader_source = malloc(shader_source_size);
  size_t read = 0;

  while (!feof(f) && !ferror(f)) {
	if (read + BUFSIZ > shader_source_size) {
	  shader_source_size *= 2;
	  shader_source = realloc(shader_source, shader_source_size);
	}
	read += fread(shader_source + read, 1, BUFSIZ, f);
  }

  fclose(f);
  shader_source = realloc(shader_source, read + 1);
  shader_source[read] = '\0';

  shader_load_text(shdr, shader_source, type);

  free(shader_source);
}

void
shader_link(shader *shdr) {
  glLinkProgram(shdr->program);

  GLint status;
  glGetProgramiv(shdr->program, GL_LINK_STATUS, &status);
  if (!status) {
	printf("Error while linking Shader Program\n");
	print_log(shdr->program);
	shader_free(shdr);
	exit(EXIT_FAILURE);
  }
}

GLint
shader_get_uniform_location(const shader *shdr, const char *name) {
  GLint loc = glGetUniformLocation(shdr->program, name);
  if (loc == -1) {
	printf("Uniform location could not be found\n");
	exit(EXIT_FAILURE);
  }
  return loc;
}

GLint
shader_get_attrib_location(const shader *shdr, const char *name) {
  GLint loc = glGetAttribLocation(shdr->program, name);
  if (loc == -1) {
	printf("Attrib location could not be found\n");
	exit(EXIT_FAILURE);
  }
  return loc;
}

void
shader_use(shader *shdr) {
  glUseProgram(shdr->program);
}

void
shader_free(const shader *const shdr) {
  glDeleteShader(shdr->vertex_shader);
  glDeleteShader(shdr->fragment_shader);
  glDeleteProgram(shdr->program);
  free(shdr);
}