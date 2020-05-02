#include <stdio.h>

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "client/text_render.h"

// Function prototypes
void error_callback(int error, const char *description);
void key_callback(GLFWwindow *window, int key, int scancode, int action,
				  int mode);

// Window dimensions
const GLuint WIDTH = 640, HEIGHT = 480;

// The MAIN function, from here we start the application and run the game loop
int
main(int argc, char **argv) {
  printf("Starting GLFW context\n");

  // Init GLFW
  glfwInit();

  // Set all the required options for GLFW
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

  // Create a GLFWwindow object that we can use for GLFW's functions
  GLFWwindow *window =
		  glfwCreateWindow(WIDTH, HEIGHT, "LearnOpenGL", NULL, NULL);
  if (window == NULL) {
	printf("Failed to create GLFW window\n");
	glfwTerminate();
	return -1;
  }

  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
	printf("Failed to initialize OpenGL context\n");
	glfwTerminate();
	return -1;
  }

  // Set the required callback functions
  glfwSetErrorCallback(error_callback);
  glfwSetKeyCallback(window, key_callback);

  printf("This is OpenGL version %s with renderer %s\n",
		 glGetString(GL_VERSION), glGetString(GL_RENDERER));

  text_state ts;
  text_render_init(&ts);

  // Define the viewport dimensions
  glViewport(0, 0, WIDTH, HEIGHT);

  // Game loop
  unsigned short long_boi1 = 0;
  unsigned short long_boi2 = 0;
  unsigned short long_boi3 = 0;
  while (!glfwWindowShouldClose(window)) {
	long_boi1 += 31;
	long_boi1 %= 50000;

	long_boi2 += 29;
	long_boi2 %= 50000;

	long_boi3 += 37;
	long_boi3 %= 50000;

	// Render
	// Clear the colorbuffer
	glClearColor(long_boi1 / 50000.0f, long_boi2 / 50000.0f,
				 long_boi3 / 50000.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// Swap the screen buffers (with vsync)
	glfwSwapInterval(1);
	glfwSwapBuffers(window);

	// Check if any events have been activated (key pressed, mouse moved etc.)
	// and call corresponding response functions
	glfwPollEvents();
  }

  // Terminates GLFW, clearing any resources allocated by GLFW.
  glfwTerminate();
  return 0;
}

void
error_callback(int error, const char *description) {
  fprintf(stderr, "Error (%d): %s\n", error, description);
}

// Is called whenever a key is pressed/released via GLFW
void
key_callback(GLFWwindow *window, int key, int scancode, int action, int mode) {
  printf("key_callback: key=%d scancode=%d action=%d mode=%d\n", key, scancode,
		 action, mode);
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	glfwSetWindowShouldClose(window, GL_TRUE);
}
