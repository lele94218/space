#version 330 core

in vec3 frag_pos;
in vec2 tex_coords;
in mat3 TBN;

out vec4 frag_color;

// Textures
uniform sampler2D texture_albedo;    // unit 0
uniform sampler2D texture_specular;  // unit 1 (used as metallic)
uniform sampler2D texture_normal;    // unit 2
uniform sampler2D texture_roughness; // unit 3
uniform sampler2D texture_ao;        // unit 4

// Texture availability flags
uniform int use_normal_map  = 0;
uniform int use_roughness   = 0;
uniform int use_ao          = 0;
uniform int use_metallic    = 0;

// Fallback PBR material values (used when textures are unavailable)
uniform float u_metallic  = 0.0;
uniform float u_roughness = 0.5;
uniform float u_ao        = 1.0;

uniform vec3 light_pos;
uniform vec3 light_color;
uniform vec3 view_pos;

const float PI = 3.14159265359;

// Normal Distribution Function — GGX/Trowbridge-Reitz
float DistributionGGX(vec3 N, vec3 H, float roughness) {
  float a  = roughness * roughness;
  float a2 = a * a;
  float NdotH  = max(dot(N, H), 0.0);
  float NdotH2 = NdotH * NdotH;
  float denom = (NdotH2 * (a2 - 1.0) + 1.0);
  return a2 / (PI * denom * denom);
}

// Geometry Function — Smith's Schlick-GGX
float GeometrySchlickGGX(float NdotV, float roughness) {
  float r = roughness + 1.0;
  float k = (r * r) / 8.0;
  return NdotV / (NdotV * (1.0 - k) + k);
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
  float NdotV = max(dot(N, V), 0.0);
  float NdotL = max(dot(N, L), 0.0);
  return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

// Fresnel — Schlick approximation
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
  return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
  // --- Sample textures ---
  vec3 albedo = texture(texture_albedo, tex_coords).rgb;

  float metallic  = (use_metallic  == 1) ? texture(texture_specular,  tex_coords).r : u_metallic;
  float roughness = (use_roughness == 1) ? texture(texture_roughness, tex_coords).r : u_roughness;
  float ao        = (use_ao        == 1) ? texture(texture_ao,        tex_coords).r : u_ao;

  // Normal
  vec3 N;
  if (use_normal_map == 1) {
    N = texture(texture_normal, tex_coords).rgb;
    N = normalize(N * 2.0 - 1.0);
    N = normalize(TBN * N);
  } else {
    N = normalize(TBN[2]);
  }

  vec3 V = normalize(view_pos - frag_pos);

  // F0: base reflectivity (0.04 for dielectrics, albedo-tinted for metals)
  vec3 F0 = mix(vec3(0.04), albedo, metallic);

  // --- Cook-Torrance BRDF ---
  vec3 Lo = vec3(0.0);

  // Single point light
  vec3 L = normalize(light_pos - frag_pos);
  vec3 H = normalize(V + L);
  float dist  = length(light_pos - frag_pos);
  float attenuation = 1.0 / (dist * dist);
  vec3 radiance = light_color * attenuation;

  float NdotL = max(dot(N, L), 0.0);

  // Cook-Torrance specular terms
  float D = DistributionGGX(N, H, roughness);
  float G = GeometrySmith(N, V, L, roughness);
  vec3  F = FresnelSchlick(max(dot(H, V), 0.0), F0);

  vec3 numerator   = D * G * F;
  float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
  vec3 specular = numerator / denominator;

  // Energy conservation: kd = (1 - F) * (1 - metallic)
  vec3 kd = (vec3(1.0) - F) * (1.0 - metallic);

  Lo += (kd * albedo / PI + specular) * radiance * NdotL;

  // Ambient (IBL approximation: constant + AO)
  vec3 ambient = vec3(0.03) * albedo * ao;
  vec3 color = ambient + Lo;

  // Tone mapping (Reinhard) + gamma correction
  color = color / (color + vec3(1.0));
  color = pow(color, vec3(1.0 / 2.2));

  frag_color = vec4(color, 1.0);
}
