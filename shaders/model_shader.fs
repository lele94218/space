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
in vec3 normal;
in vec2 tex_coords;

out vec4 frag_color;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform Material material;
uniform Light light;
uniform vec3 view_pos;

void main() {
  // ambient
  vec3 ambient = light.ambient * texture(texture_diffuse1, tex_coords).rgb;

  // diffuse
  vec3 norm = normalize(normal);
  vec3 light_dir = normalize(light.position - frag_pos);
  float diff = max(dot(norm, light_dir), 0.0);
  vec3 diffuse = light.diffuse * diff * texture(texture_diffuse1, tex_coords).rgb;

  // specular
  vec3 view_dir = normalize(view_pos - frag_pos);
  vec3 reflect_dir = reflect(-light_dir, norm);
  float spec = pow(max(dot(view_dir, reflect_dir), 0.0), material.shininess);
  vec3 specular = light.specular * spec * texture(texture_specular1, tex_coords).rgb;

  vec3 result = ambient + diffuse + specular;
  frag_color = vec4(result, 1.0);
}
