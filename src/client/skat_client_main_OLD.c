#include <stdio.h>

#include <glad/glad.h>
//#include <GL/gl.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <GL/glu.h>

#include "client/text_render.h"

// Function prototypes
void error_callback(int error, const char *description);
void key_callback(GLFWwindow *window, int key, int scancode, int action,
				  int mode);
void resize_callback(GLFWwindow *window, int width, int height);

// Window dimensions
const GLuint WIDTH = 640, HEIGHT = 480;

// The MAIN function, from here we start the application and run the game loop
int
start_OLD(void) {
  printf("Starting GLFW context\n");

  // Init GLFW
  glfwInit();

  // Set all the required options for GLFW
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

  // Create a GLFWwindow object that we can use for GLFW's functions
  GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Skat", NULL, NULL);
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
  glfwSetWindowSizeCallback(window, resize_callback);
  glfwSetKeyCallback(window, key_callback);

  printf("This is OpenGL version %s with renderer %s\n",
		 glGetString(GL_VERSION), glGetString(GL_RENDERER));

  // text_state ts;
  // text_render_init(&ts);

  // Define the viewport dimensions
  // glViewport(0, 0, WIDTH, HEIGHT);
  // glOrtho(0, WIDTH, 0, HEIGHT, -1, 1);
  // const float ar = WIDTH / (float) HEIGHT;

  /*glViewport(0, 0, WIDTH, HEIGHT);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-ar, ar, -1.0, 1.0, 2.0, 100.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();*/

  // glMatrixMode(GL_PROJECTION);
  // glLoadIdentity();

  glEnable(GL_DEPTH_TEST);

  /*glMatrixMode(GL_PROJECTION); // Switch to the projection matrix so that we
  can manipulate how our scene is viewed glLoadIdentity(); // Reset the
  projection matrix to the identity matrix so that we don't get any artifacts
  (cleaning up) gluPerspective(60, (GLfloat)width / (GLfloat)height, 1.0,
  100.0); // Set the Field of view angle (in degrees), the aspect ratio of our
  window, and the new and far planes
  glMatrixMode(GL_MODELVIEW); // Switch back to the model view matrix, so that
  we can start drawing shapes correctly*/

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
	/*glClearColor(long_boi1 / 50000.0f, long_boi2 / 50000.0f,
				 long_boi3 / 50000.0f, 1.0f);*/
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);// Switch to the drawing perspective
	glLoadIdentity();          // Reset the drawing perspective

	glBegin(GL_QUADS);
	glColor3ub(0xff, 0, 0);
	glVertex3f(-0.7f, -1.5f, -5.0f);
	glVertex3f(0.7f, -1.5f, -5.0f);
	glVertex3f(0.4f, -0.5f, -5.0f);
	glVertex3f(-0.4f, -0.5f, -5.0f);
	glEnd();
	/*glPushMatrix();
	glLoadIdentity();
	// glRotatef(cnt1,0,0,1);
	// glScalef(1,.8+.3*cos(cnt1/5),1);
	// glTranslatef(-180,0,0);
	glTranslatef(0, 0, 0);
	text_render_print(&ts, 0, 0, "Active FreeType Text - %7.2f", long_boi1);
	glPopMatrix();*/

	// glfwSwapInterval(1); // Swap the screen buffers (with vsync)
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

void
resize_callback(GLFWwindow *window, int width, int height) {
  // Tell OpenGL how to convert from coordinates to pixel values
  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);// Switch to setting the camera perspective

  // Set the camera perspective
  glLoadIdentity();                                 // reset the camera
  gluPerspective(45.0f,                             // camera angle
				 (GLfloat) width / (GLfloat) height,// The width to height ratio
				 1.0f,   // The near z clipping coordinate
				 100.0f);// The far z clipping coordinate
}
