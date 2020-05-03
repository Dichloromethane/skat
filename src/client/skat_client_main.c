#include "glad/glad.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "linmath.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static const struct {
  float x, y;
  float r, g, b;
} vertices[3] = {{-0.6f, -0.4f, 1.f, 0.f, 0.f},
				 {0.6f, -0.4f, 0.f, 1.f, 0.f},
				 {0.f, 0.6f, 0.f, 1.f, 0.f}};

static const char *vertex_shader_text =
		"#version 110\n"
		"uniform mat4 MVP;\n"
		"attribute vec3 vCol;\n"
		"attribute vec2 vPos;\n"
		"varying vec3 color;\n"
		"void main()\n"
		"{\n"
		"    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
		"    color = vCol;\n"
		"}\n";

static const char *fragment_shader_text =
		"#version 110\n"
		"varying vec3 color;\n"
		"void main()\n"
		"{\n"
		"    //vec2 res = vec2(640.0, 480.0);\n"
		"    //vec2 st = gl_FragCoord.xy / res;\n"
		"    //;\n"
		"    //gl_FragColor = vec4(st.x, st.y, 0.0, 1.0);\n"
		"    gl_FragColor = vec4(color.xy, (gl_FragCoord.y - 240.0) / 60.0, 1.0);\n"
		"}\n";

static void
error_callback(int error, const char *description) {
  fprintf(stderr, "Error: %s (%d)\n", description, error);
}

static void
key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  printf("key_callback: key=%d scancode=%d action=%d mods=%d\n", key, scancode,
		 action, mods);
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int
main(void) {
  GLFWwindow *window;
  GLuint vertex_buffer, vertex_shader, fragment_shader, program;
  GLint mvp_location, vpos_location, vcol_location;

  glfwSetErrorCallback(error_callback);

  if (!glfwInit())
	exit(EXIT_FAILURE);

  // DO NOT UPDATE: indirect drawing via X Forwarding does not support higher GL
  // versions :(
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  // glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

  window = glfwCreateWindow(640, 480, "Simple example", NULL, NULL);
  if (!window) {
	printf("Failed to create GLFW window\n");
	glfwTerminate();
	exit(EXIT_FAILURE);
  }

  glfwSetKeyCallback(window, key_callback);

  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
	printf("Failed to initialize OpenGL context\n");
	glfwTerminate();
	exit(EXIT_FAILURE);
  }

  printf("This is OpenGL version %s with renderer %s\n",
		 glGetString(GL_VERSION), glGetString(GL_RENDERER));

  glfwSwapInterval(1);

  // NOTE: OpenGL error checks have been omitted for brevity

  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  GLint status = GL_FALSE;

  vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
  glCompileShader(vertex_shader);

  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &status);
  if (status) {
	printf("Vertex Shader compile success\n");
  } else {
	GLint len = 0;
	glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &len);
	GLchar *text = calloc(len, sizeof(GLchar));

	glGetShaderInfoLog(vertex_shader, len, &len, text);
	printf("Vertex Shader (%d):\n%s\n", len, text);
	free(text);
	glDeleteShader(vertex_shader);
	exit(EXIT_FAILURE);
  }

  fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
  glCompileShader(fragment_shader);
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &status);
  if (status) {
	printf("Fragment Shader compile success\n");
  } else {
	GLint len = 0;
	glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &len);
	GLchar *text = calloc(len, sizeof(GLchar));

	glGetShaderInfoLog(fragment_shader, len, &len, text);
	printf("Fragment Shader Error: %s\n", text);
	free(text);
	glDeleteShader(fragment_shader);
	exit(EXIT_FAILURE);
  }

  program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (status) {
	printf("Shader Program link success\n");
  } else {
	GLint len = 0;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
	GLchar *text = calloc(len, sizeof(GLchar));

	glGetProgramInfoLog(program, len, &len, text);
	printf("Shader Program Error: %s\n", text);
	free(text);
	glDeleteProgram(program);
	exit(EXIT_FAILURE);
  }

  mvp_location = glGetUniformLocation(program, "MVP");
  vpos_location = glGetAttribLocation(program, "vPos");
  vcol_location = glGetAttribLocation(program, "vCol");

  glEnableVertexAttribArray(vpos_location);
  glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE,
						sizeof(vertices[0]), (void *) 0);
  glEnableVertexAttribArray(vcol_location);
  glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE,
						sizeof(vertices[0]), (void *) (sizeof(float) * 2));

  while (!glfwWindowShouldClose(window)) {
	float ratio;
	int width, height;
	mat4x4 m, p, mvp;

	glfwGetFramebufferSize(window, &width, &height);
	ratio = width / (float) height;

	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT);

	mat4x4_identity(m);
	mat4x4_rotate_Z(m, m, (float) glfwGetTime());
	mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
	mat4x4_mul(mvp, p, m);

	glUseProgram(program);
	glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat *) mvp);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glfwSwapBuffers(window);
	glfwPollEvents();
  }

  glfwDestroyWindow(window);

  glfwTerminate();
  exit(EXIT_SUCCESS);
}
