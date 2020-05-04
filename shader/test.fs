#version 110

varying vec3 color;

void main()
{
    //vec2 res = vec2(640.0, 480.0);
    //vec2 st = gl_FragCoord.xy / res;
    //gl_FragColor = vec4(st.x, st.y, 0.0, 1.0);
    gl_FragColor = vec4(color.xy, (gl_FragCoord.y - 240.0) / 60.0, 1.0);
}
