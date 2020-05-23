#version 140

uniform mat4 projection;
uniform mat4 model;

attribute vec4 coord;

varying vec2 texpos;

void main() {
    gl_Position = projection * model * vec4(coord.xy, 0, 1);
    texpos = coord.zw;
}
