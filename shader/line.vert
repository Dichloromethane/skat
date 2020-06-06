#version 140

uniform mat4 projection;
uniform mat4 model;

attribute vec2 pos;

void main() {
    gl_Position = projection * model * vec4(pos, 0, 1);
}
