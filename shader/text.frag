#version 140

uniform sampler2D tex;
uniform vec4 color;

varying vec2 texpos;

void main() {
    gl_FragColor = color * vec4(1, 1, 1, texture2D(tex, texpos).r);
}