#version 140

uniform mat4 projection;
uniform mat4 model;

attribute vec4 coord;

varying vec2 texpos;
varying float f_colorProgress;

void main() {
    gl_Position = projection * model * vec4(coord.xy, 0.0, 1.0);
    texpos = coord.zw;
    f_colorProgress = coord.x;
}
