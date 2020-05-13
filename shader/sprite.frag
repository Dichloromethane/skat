#version 140

uniform sampler2D tex;
uniform vec4 color;

varying vec2 f_texpos;

void main() {
    gl_FragColor = color * texture2D(tex, f_texpos);
}
