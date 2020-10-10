#version 140

uniform sampler2DArray tex;

varying vec3 texpos_frag;
varying float f_colorProgress;

#define PI (3.1415926538)
#define OMEGA (0.0075 * PI)

void main() {
    float r = (sin(f_colorProgress * OMEGA) + 1.0) / 2.0;
    float b = (sin(f_colorProgress * OMEGA + 2.0/3.0 * PI) + 1.0) / 2.0;
    float g = (sin(f_colorProgress * OMEGA + 4.0/3.0 * PI) + 1.0) / 2.0;
    vec4 color = vec4(r, b, g, 1);
    gl_FragColor = color * vec4(1, 1, 1, texture(tex, texpos_frag).r);
}
