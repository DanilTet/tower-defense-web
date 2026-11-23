#version 460 core

out vec4 frag_color;

in vec2 v_tex_coords;
in vec4 v_color;

uniform sampler2D u_texture;

void main() {
   frag_color = texture(u_texture, v_tex_coords) * v_color;
}