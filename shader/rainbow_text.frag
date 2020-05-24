#version 140

uniform sampler2D tex;

varying vec2 texpos;
varying float f_colorProgress;

#define PI 3.1415926538

void main() {
    float omega = 7.5*PI;
    float r = (sin(f_colorProgress * omega) + 1.0) / 2.0;
    float b = (sin(f_colorProgress * omega + 2.0/3.0 * PI) + 1.0) / 2.0;
    float g = (sin(f_colorProgress * omega + 4.0/3.0 * PI) + 1.0) / 2.0;
    vec4 color = vec4(r, b, g, 1.0);
    gl_FragColor = color * vec4(1.0, 1.0, 1.0, texture2D(tex, texpos).r);
}
