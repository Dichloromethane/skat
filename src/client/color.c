#include "client/color.h"

void
color_to_rgba_f(color col, float *rgba) {
  rgba[0] = (float) col.r / 255.0f;
  rgba[1] = (float) col.g / 255.0f;
  rgba[2] = (float) col.b / 255.0f;
  rgba[3] = (float) col.a / 255.0f;
}
