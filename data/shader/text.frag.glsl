#version 120

uniform sampler2D u_texture;
uniform vec4 u_color, u_bgcolor;
varying vec2 v_texcoord;

void main() {
    gl_FragColor = mix(u_bgcolor, u_color,
                       texture2D(u_texture, v_texcoord).r);
}
