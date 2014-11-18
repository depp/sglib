#version 120

attribute vec4 a_vert;

uniform vec2 u_vertoff;
uniform vec2 u_vertscale;
uniform vec2 u_texscale;

varying vec2 v_texcoord;

void main() {
    v_texcoord = a_vert.zw * u_texscale;
    gl_Position = vec4((a_vert.xy + u_vertoff) * u_vertscale, 0.0, 1.0);
}
