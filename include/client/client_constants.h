#pragma once

#include "glad/glad.h"

#define WIDTH  (1280)
#define HEIGHT (720)
//#define RAINBOW_TEXT 1

extern float screen_width;
extern float screen_height;

typedef struct {
  GLfloat x;
  GLfloat y;
  GLfloat s;
  GLfloat t;
} point;

typedef struct {
  GLfloat x, y;
  GLfloat r, g, b;
} vertex2d;
