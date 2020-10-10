#version 140

uniform mat4 projection;
uniform mat4 model;

attribute vec2 coord;
attribute vec3 texpos;

varying vec3 texpos_frag;

void main() {
    gl_Position = projection * model * vec4(coord, 0, 1);
    texpos_frag = texpos;
}
