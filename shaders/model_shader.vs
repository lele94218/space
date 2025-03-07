#version 330 core
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_tex_coords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 normal;
out vec3 frag_pos;
out vec2 tex_coords;

void main() {
  gl_Position = projection * view * model * vec4(a_pos, 1.0);
  tex_coords = a_tex_coords;
  normal = mat3(transpose(inverse(model))) * a_normal;
  frag_pos = vec3(model * vec4(a_pos, 1.0));
}
