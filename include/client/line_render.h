#pragma once

#define _GNU_SOURCE

#include "client/color.h"

void line_render_init(void);

void render_line(color col, float start_x, float start_y, float end_x,
				 float end_y);

void render_box(color col, float start_x, float start_y, float width,
				float height);
