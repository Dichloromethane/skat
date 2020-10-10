#version 140

uniform sampler2DArray tex;
uniform vec4 color;

varying vec3 texpos_frag;

void main() {
    gl_FragColor = color * vec4(1, 1, 1, texture(tex, texpos_frag).r);
}
