#version 330 core
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_tex_coords;
layout (location = 3) in vec3 a_tangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 frag_pos;
out vec2 tex_coords;
out mat3 TBN;        // tangent space -> world space matrix

void main() {
  gl_Position = projection * view * model * vec4(a_pos, 1.0);
  tex_coords = a_tex_coords;
  frag_pos = vec3(model * vec4(a_pos, 1.0));

  // Build TBN matrix in world space
  mat3 normal_matrix = transpose(inverse(mat3(model)));
  vec3 T = normalize(normal_matrix * a_tangent);
  vec3 N = normalize(normal_matrix * a_normal);
  // Re-orthogonalize T with respect to N (Gram-Schmidt)
  T = normalize(T - dot(T, N) * N);
  vec3 B = cross(N, T);
  TBN = mat3(T, B, N);
}
