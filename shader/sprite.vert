#version 140

uniform mat4 model;
uniform mat4 projection;

attribute vec2 pos;
attribute vec2 texpos;

varying vec2 f_texpos;

void main() {
    gl_Position = projection * model * vec4(pos, 0.0, 1.0);
    f_texpos = texpos;
}
