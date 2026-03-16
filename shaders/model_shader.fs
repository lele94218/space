#version 330 core

struct Light {
  vec3 position;
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

struct Material {
  sampler2D diffuse;
  sampler2D specular;
  float shininess;
};

in vec3 frag_pos;
in vec2 tex_coords;
in mat3 TBN;

out vec4 frag_color;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;
uniform Material material;
uniform Light light;
uniform vec3 view_pos;
uniform vec3 default_color = vec3(1.0, 1.0, 1.0);
uniform vec3 base_color_factor = vec3(1.0, 1.0, 1.0);
uniform int texture_sample = 1;
uniform int use_normal_map = 0;

void main() {
  // Normal: sample from normal map or use interpolated vertex normal
  vec3 norm;
  if (use_normal_map == 1) {
    // Sample normal map, transform from [0,1] to [-1,1] tangent space
    norm = texture(texture_normal1, tex_coords).rgb;
    norm = normalize(norm * 2.0 - 1.0);
    // Transform from tangent space to world space
    norm = normalize(TBN * norm);
  } else {
    norm = normalize(TBN[2]); // TBN[2] is the world-space vertex normal
  }

  vec3 ambient, diffuse, specular;

  // Resolve albedo: texture * base_color_factor (or just factor if no texture)
  vec3 albedo;
  if (texture_sample == 1) {
    albedo = texture(texture_diffuse1, tex_coords).rgb * base_color_factor;
  } else {
    albedo = base_color_factor;
  }

  // Ambient
  ambient = light.ambient * albedo;

  // Diffuse
  vec3 light_dir = normalize(light.position - frag_pos);
  float diff = max(dot(norm, light_dir), 0.0);
  diffuse = light.diffuse * diff * albedo;

  // Specular (Blinn-Phong)
  vec3 view_dir = normalize(view_pos - frag_pos);
  vec3 halfway_dir = normalize(light_dir + view_dir);
  float spec = pow(max(dot(norm, halfway_dir), 0.0), material.shininess);
  if (texture_sample == 1) {
    specular = light.specular * spec * texture(texture_specular1, tex_coords).rgb;
  } else {
    specular = light.specular * spec * vec3(0.3);
  }

  vec3 result = ambient + diffuse + specular;
  frag_color = vec4(result, 1.0);
}
