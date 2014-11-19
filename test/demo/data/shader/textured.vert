#version 120

attribute vec2 a_loc;
attribute vec2 a_texcoord;

uniform vec2 u_vertoff;
uniform vec2 u_vertscale;
uniform vec2 u_texscale;

varying vec2 v_texcoord;

void main() {
    gl_Position = vec4((a_loc + u_vertoff) * u_vertscale, 0.0, 1.0);
    v_texcoord = a_texcoord * u_texscale;
}
