#pragma once

#include "glad/glad.h"

typedef struct {
  GLfloat x;
  GLfloat y;
} vertex2f;

typedef struct {
  GLfloat x;
  GLfloat y;
  GLfloat s;
  GLfloat t;
} vertex2f_st;

typedef struct {
  GLfloat x;
  GLfloat y;
  GLfloat s;
  GLfloat t;
  GLfloat l;// texture layer
} vertex2f_stl;

typedef struct {
  GLfloat x;
  GLfloat y;
  GLfloat r;
  GLfloat g;
  GLfloat b;
} vertex2f_rgb;
