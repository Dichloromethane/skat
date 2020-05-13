#version 140

uniform mat4 projection;

attribute vec4 coord;

varying vec2 texpos;

void main(void) {
    gl_Position = projection * vec4(coord.xy, 0, 1);
    texpos = coord.zw;
}
