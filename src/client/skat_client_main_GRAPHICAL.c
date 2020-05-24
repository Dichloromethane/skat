#include "glad/glad.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "client/client_constants.h"
#include "client/linmath.h"
#include "client/shader.h"
#include "client/text_render.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

float screen_width = WIDTH;
float screen_height = HEIGHT;

static void
error_callback(int error, const char *description) {
  fprintf(stderr, "Error: %s (%d)\n", description, error);
}

static void
resize_callback(GLFWwindow *window, int width, int height) {
  if (height == 0)
	height = 1;// To prevent divide by 0

  GLfloat desiredAspect = (GLfloat) HEIGHT / WIDTH;
  GLfloat aspect = (GLfloat) height / (GLfloat) width;

  GLfloat actualWidth, actualHeight, x, y;

  if (aspect == desiredAspect) {// perfect ratio
	actualWidth = width;
	actualHeight = height;
	x = 0;
	y = 0;
  } else if (aspect < desiredAspect) {// window too high
	actualWidth = (float) height / desiredAspect;
	actualHeight = height;
	x = ((float) width - actualWidth) / 2.0f;
	y = 0;
  } else {// window too broad
	actualWidth = width;
	actualHeight = (float) width * desiredAspect;
	x = 0;
	y = ((float) height - actualHeight) / 2.0f;
  }

  glViewport(x, y, actualWidth, actualHeight);

  screen_width = (float) width;
  screen_height = (float) height;
  text_render_rescale(screen_width, screen_height);
}

static void
key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  printf("key_callback: key=%d scancode=%d action=%d mods=%d\n", key, scancode,
		 action, mods);
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int
start_GRAPHICAL(void) {
  GLFWwindow *window;
  GLuint vertex_buffer;
  shader *program;
  GLint proj_location, model_location, vpos_location, vcol_location;

  glfwSetErrorCallback(error_callback);

  if (!glfwInit())
	exit(EXIT_FAILURE);

  // DO NOT UPDATE: indirect drawing via X Forwarding does not support higher GL
  // versions :(
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
  // glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

  window = glfwCreateWindow(WIDTH, HEIGHT, "Skat", NULL, NULL);
  if (!window) {
	printf("Failed to create GLFW window\n");
	glfwTerminate();
	exit(EXIT_FAILURE);
  }

  glfwSetWindowSizeCallback(window, resize_callback);
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
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  text_render_init();

  vertex2d triangle_vertices[3] = {{0.0f, -0.6f, 1.f, 0.f, 0.f},
								   {-0.6f, 0.4f, 0.f, 1.f, 0.f},
								   {0.6f, 0.4f, 0.f, 0.f, 1.f}};
  // NOTE: OpenGL error checks have been omitted for brevity
  program = shader_create_load_file("./shader/test");
  shader_use(program);

  proj_location = shader_get_uniform_location(program, "projection");
  model_location = shader_get_uniform_location(program, "model");
  vpos_location = shader_get_attrib_location(program, "vPos");
  vcol_location = shader_get_attrib_location(program, "vCol");

  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_vertices), triangle_vertices,
			   GL_STATIC_DRAW);

  mat4x4 projection;
  mat4x4_identity(projection);
  mat4x4_ortho(projection, 0, WIDTH, HEIGHT, 0, -1, 1);
  glUniformMatrix4fv(proj_location, 1, GL_FALSE, (const GLfloat *) projection);

  while (!glfwWindowShouldClose(window)) {
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	shader_use(program);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

	glEnableVertexAttribArray(vpos_location);
	glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE,
						  sizeof(vertex2d), 0);
	glEnableVertexAttribArray(vcol_location);
	glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE,
						  sizeof(vertex2d),
						  (const void *) (sizeof(GLfloat) * 2));

	mat4x4 model;
	mat4x4_identity(model);
	mat4x4_translate_in_place(model, WIDTH / 2.0f, HEIGHT / 2.0f, 0);
	mat4x4_rotate_Z(model, model, (float) glfwGetTime());
	mat4x4_scale_aniso(model, model, HEIGHT / 2.0f, HEIGHT / 2.0f, 1);
	glUniformMatrix4fv(model_location, 1, GL_FALSE, (const GLfloat *) model);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	text_render_print(TRL_TOP_LEFT, GREEN, 10.0f, 0.0f, 1.0f,
					  "TeQuBrFoJuOvThLaDo!");
	text_render_print(TRL_TOP_LEFT, BLUE, 10.0f, 36.0f, 1.0f,
					  "TeQuBrFoJuOvThLaDo");
	text_render_debug(1.0f, 175.0f, 1.0f);

	glfwSwapBuffers(window);
	glfwPollEvents();
  }

  glfwDestroyWindow(window);

  glfwTerminate();
  exit(EXIT_SUCCESS);
}
