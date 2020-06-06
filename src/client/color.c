#include "client/color.h"

void
color_to_rgba_f(color col, float *rgba) {
  color_to_rgb_f(col, rgba);
  rgba[3] = (float) col.a / 255.0f;
}

void
color_to_rgb_f(color col, float *rgb) {
  rgb[0] = (float) col.r / 255.0f;
  rgb[1] = (float) col.g / 255.0f;
  rgb[2] = (float) col.b / 255.0f;
}
