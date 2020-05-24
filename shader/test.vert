#version 140

uniform mat4 projection;
uniform mat4 model;

attribute vec3 vCol;
attribute vec2 vPos;

varying vec3 color;

void main() {
    gl_Position = projection * model * vec4(vPos, 0.0, 1.0);
    color = vCol;
}
