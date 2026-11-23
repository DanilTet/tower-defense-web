#version 460 core
layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_tex_coords;
layout(location = 2) in vec4 in_color;

out vec2 v_tex_coords;
out vec4 v_color;

uniform mat4 u_projection;

void main() {
    gl_Position = u_projection * vec4(in_position, 0.0, 1.0);
    v_tex_coords = in_tex_coords;
    v_color = in_color;
}