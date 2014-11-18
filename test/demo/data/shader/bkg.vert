#version 120

attribute vec2 a_loc;

uniform vec2 u_texoff;
uniform mat2 u_texmat;

varying vec2 v_texcoord;

void main() {
    gl_Position = vec4(a_loc, 0.0, 1.0);
    v_texcoord = u_texoff + u_texmat * a_loc;
}
