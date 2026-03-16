#version 330 core
out vec4 frag_color;

in vec3 local_pos;

uniform samplerCube environment_map;

const float PI = 3.14159265359;

void main() {
  // The irradiance integral over the hemisphere surrounding a surface normal N.
  // We integrate the environment radiance for diffuse indirect lighting.
  vec3 N = normalize(local_pos);

  vec3 irradiance = vec3(0.0);

  // Build an orthonormal basis around N
  vec3 up    = vec3(0.0, 1.0, 0.0);
  vec3 right = normalize(cross(up, N));
  up         = normalize(cross(N, right));

  float sample_delta = 0.025;
  float num_samples  = 0.0;
  for (float phi = 0.0; phi < 2.0 * PI; phi += sample_delta) {
    for (float theta = 0.0; theta < 0.5 * PI; theta += sample_delta) {
      // Spherical to Cartesian (in tangent space)
      vec3 tangent_sample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
      // Tangent to world space
      vec3 sample_vec = tangent_sample.x * right + tangent_sample.y * up + tangent_sample.z * N;

      irradiance += texture(environment_map, sample_vec).rgb * cos(theta) * sin(theta);
      num_samples++;
    }
  }

  irradiance = PI * irradiance * (1.0 / float(num_samples));
  frag_color = vec4(irradiance, 1.0);
}
