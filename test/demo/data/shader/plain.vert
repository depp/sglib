#version 120

attribute vec2 a_loc;

uniform vec2 u_vertoff;
uniform vec2 u_vertscale;

void main() {
    gl_Position = vec4((a_loc + u_vertoff) * u_vertscale, 0.0, 1.0);
}
