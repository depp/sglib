#version 120

uniform sampler2D u_tex1;
uniform sampler2D u_tex2;

varying vec2 v_texcoord;

uniform float u_fade;

void main() {
    gl_FragColor = mix(
        texture2D(u_tex1, v_texcoord),
        texture2D(u_tex2, v_texcoord),
        u_fade);
}
