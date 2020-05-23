#include "glad/glad.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "client/linmath.h"
#include "client/shader.h"
#include "client/text_render.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static const struct {
  float x, y;
  float r, g, b;
} vertices[3] = {{-0.6f, -0.4f, 1.f, 0.f, 0.f},
				 {0.6f, -0.4f, 0.f, 1.f, 0.f},
				 {0.f, 0.6f, 0.f, 1.f, 0.f}};

#define WIDTH  (640)
#define HEIGHT (480)
float screen_width = WIDTH;
float screen_height = HEIGHT;

static void
error_callback(int error, const char *description) {
  fprintf(stderr, "Error: %s (%d)\n", description, error);
}

static void
resize_callback(GLFWwindow *window, int width, int height) {
  /*int side = width < height ? width : height;
  glViewport((width - side) / 2, (height - side) / 2, side, side);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-0.05, +1.45, -0.05, +2.75, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();*/

  // glViewport(0, 0, width, height);

  /*float aspect = (float) width / (float) height;
  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0f, -aspect, aspect, 0.0f, -1.0f, 1.0f);*/

  // Compute aspect ratio of the new window
  if (height == 0)
	height = 1;// To prevent divide by 0
  GLfloat aspect = (GLfloat) width / (GLfloat) height;
  // Set the viewport to cover the new window
  glViewport(0, 0, width, height);
  // glViewport(0, 0, 400, 400);

  // Set the aspect ratio of the clipping area to match the viewport
  /*glMatrixMode(GL_PROJECTION);// To operate on the Projection matrix
  glLoadIdentity();           // Reset the projection matrix

  GLdouble clipAreaXLeft, clipAreaXRight, clipAreaYBottom, clipAreaYTop;
   GLfloat minX = -1, maxX = 1, minY = -1, maxY = 1;
  //GLfloat minX = 0, maxX = 1, minY = 0, maxY = 1;

  if (width >= height) {
	clipAreaXLeft = minX * aspect;
	clipAreaXRight = maxX * aspect;
	clipAreaYBottom = minY;
	clipAreaYTop = maxY;
  } else {
	clipAreaXLeft = minX;
	clipAreaXRight = maxX;
	clipAreaYBottom = minY / aspect;
	clipAreaYTop = maxY / aspect;
  }
  glOrtho(clipAreaXLeft, clipAreaXRight, clipAreaYBottom, clipAreaYTop, -1.0,
		  1.0);*/

  screen_width = (float) width;
  screen_height = (float) height;
  text_render_rescale(screen_width, screen_height);
}
static GLfloat xrunner = 0;
static void
key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  printf("key_callback: key=%d scancode=%d action=%d mods=%d\n", key, scancode,
		 action, mods);
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	glfwSetWindowShouldClose(window, GLFW_TRUE);
  if (key == GLFW_KEY_Y && action == GLFW_PRESS)
	xrunner -= 25.0f;
}

int
start_GRAPHICAL(void) {
  GLFWwindow *window;
  GLuint vertex_buffer;
  shader *program;
  GLint mvp_location, vpos_location, vcol_location;

  glfwSetErrorCallback(error_callback);

  if (!glfwInit())
	exit(EXIT_FAILURE);

  // DO NOT UPDATE: indirect drawing via X Forwarding does not support higher GL
  // versions :(
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
  // glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

  window = glfwCreateWindow(640, 480, "Simple example", NULL, NULL);
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

  /*glViewport(0, 0, WIDTH, HEIGHT);
  glOrtho(0.0, WIDTH, HEIGHT, 0.0, -1.0, 1.0);*/

  /*// NOTE: OpenGL error checks have been omitted for brevity
  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  program = shader_create_load_file("shader/test");

  mvp_location = shader_get_uniform_location(program, "MVP");
  vpos_location = shader_get_attrib_location(program, "vPos");
  vcol_location = shader_get_attrib_location(program, "vCol");

  glEnableVertexAttribArray(vpos_location);
  glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE,
						sizeof(vertices[0]), (void *) 0);
  glEnableVertexAttribArray(vcol_location);
  glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE,
						sizeof(vertices[0]), (void *) (sizeof(float) * 2));*/

  // float sx = 2.0f / 640.0f;
  // float sy = 2.0f / 480.0f;

  while (!glfwWindowShouldClose(window)) {
	/*float ratio;
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

	shader_use(program);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat *) mvp);
	glDrawArrays(GL_TRIANGLES, 0, 3);*/

	/* White background */
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	// glOrtho(0.0f, WIDTH, HEIGHT, 0.0f, -1.0f, 0.0f);

	/* Enable blending, necessary for our alpha texture */
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	/*GLfloat black[4] = {0, 0, 0, 1};
	GLfloat red[4] = {1, 0, 0, 1};
	GLfloat transparent_green[4] = {0, 1, 0, 0.5};*/

	/* Set color to black */
	// glUniform4fv(ts.uniform_color, 1, black);

	text_render_print(-0.5f, -0.5f, 1.0f,
					  "The Quick Brown Fox Jumps Over The Lazy Dog!");
	/*text_render_print( -1 , 1 - 150 * sy, sx, sy,
					  ".ABCDEFGHIJKLMNOPQRSTUVWYZ.");
	text_render_print(-1 , 1 - 250 * sy, sx, sy,
					  ".abcdefghijklmnopqrstuvwyz.");*/
	//text_render_debug(-0.5f, -0.5f, 1.0f);

	glPopMatrix();

	glfwSwapBuffers(window);
	glfwPollEvents();
  }

  glfwDestroyWindow(window);

  glfwTerminate();
  exit(EXIT_SUCCESS);
}
