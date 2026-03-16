#version 330 core

in vec3 frag_pos;
in vec2 tex_coords;
in mat3 TBN;

out vec4 frag_color;

// Texture units (always bound, fallback = 1x1 white)
uniform sampler2D texture_albedo;           // unit 0
uniform sampler2D texture_metallic_rough;   // unit 1  G=roughness, B=metallic
uniform sampler2D texture_normal;           // unit 2
uniform sampler2D texture_occlusion;        // unit 3  R=ao
uniform sampler2D texture_emissive;         // unit 4

// Texture flags
uniform int use_metallic_rough = 0;
uniform int use_normal_map     = 0;
uniform int use_occlusion      = 0;
uniform int use_emissive       = 0;
uniform int has_albedo         = 0;

// PBR factors (multiplied with texture samples)
uniform vec4  base_color_factor    = vec4(1.0, 1.0, 1.0, 1.0);
uniform float metallic_factor      = 1.0;
uniform float roughness_factor     = 1.0;
uniform float normal_scale         = 1.0;
uniform float occlusion_strength   = 1.0;
uniform vec3  emissive_factor      = vec3(0.0);

// Alpha
uniform int   alpha_mode    = 0;   // 0=OPAQUE, 1=MASK, 2=BLEND
uniform float alpha_cutoff  = 0.5;

// Lighting
uniform vec3 light_pos;
uniform vec3 light_color;
uniform vec3 view_pos;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness) {
  float a  = roughness * roughness;
  float a2 = a * a;
  float d  = max(dot(N, H), 0.0);
  float denom = d * d * (a2 - 1.0) + 1.0;
  return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
  float r = roughness + 1.0;
  float k = (r * r) / 8.0;
  return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
  return GeometrySchlickGGX(max(dot(N, V), 0.0), roughness)
       * GeometrySchlickGGX(max(dot(N, L), 0.0), roughness);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
  return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
  // ── Albedo ────────────────────────────────────────────────────────────────
  vec4 albedo_sample = (has_albedo == 1)
      ? texture(texture_albedo, tex_coords)
      : vec4(1.0);
  vec4 base = albedo_sample * base_color_factor;

  // Alpha handling
  float alpha = base.a;
  if (alpha_mode == 1 && alpha < alpha_cutoff) discard;

  // sRGB -> linear for albedo RGB
  vec3 albedo = pow(max(base.rgb, vec3(0.0)), vec3(2.2));

  // ── Metallic / Roughness ─────────────────────────────────────────────────
  float metallic  = metallic_factor;
  float roughness = roughness_factor;
  if (use_metallic_rough == 1) {
    vec3 mr = texture(texture_metallic_rough, tex_coords).rgb;
    roughness *= mr.g;   // G channel
    metallic  *= mr.b;   // B channel
  }
  roughness = clamp(roughness, 0.04, 1.0);
  metallic  = clamp(metallic,  0.0,  1.0);

  // ── Normal ───────────────────────────────────────────────────────────────
  vec3 N;
  if (use_normal_map == 1) {
    vec3 n = texture(texture_normal, tex_coords).rgb * 2.0 - 1.0;
    n.xy *= normal_scale;
    N = normalize(TBN * normalize(n));
  } else {
    N = normalize(TBN[2]);
  }

  // ── Occlusion ────────────────────────────────────────────────────────────
  float ao = 1.0;
  if (use_occlusion == 1) {
    float r = texture(texture_occlusion, tex_coords).r;
    ao = 1.0 + occlusion_strength * (r - 1.0);
  }

  // ── Cook-Torrance BRDF ───────────────────────────────────────────────────
  vec3 V  = normalize(view_pos - frag_pos);
  vec3 F0 = mix(vec3(0.04), albedo, metallic);

  vec3 L = normalize(light_pos - frag_pos);
  vec3 H = normalize(V + L);
  float dist        = length(light_pos - frag_pos);
  float attenuation = 1.0 / (dist * dist);
  vec3  radiance    = light_color * attenuation;
  float NdotL       = max(dot(N, L), 0.0);

  float D = DistributionGGX(N, H, roughness);
  float G = GeometrySmith(N, V, L, roughness);
  vec3  F = FresnelSchlick(max(dot(H, V), 0.0), F0);

  vec3 spec = (D * G * F) / (4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001);
  vec3 kd   = (vec3(1.0) - F) * (1.0 - metallic);
  vec3 Lo   = (kd * albedo / PI + spec) * radiance * NdotL;

  // ── Ambient + AO ─────────────────────────────────────────────────────────
  vec3 color = vec3(0.03) * albedo * ao + Lo;

  // ── Emissive ─────────────────────────────────────────────────────────────
  if (use_emissive == 1) {
    vec3 emissive = texture(texture_emissive, tex_coords).rgb;
    color += pow(emissive, vec3(2.2)) * emissive_factor;
  } else {
    color += emissive_factor;
  }

  // ── Tone mapping + gamma ─────────────────────────────────────────────────
  color = color / (color + vec3(1.0));
  color = pow(color, vec3(1.0 / 2.2));

  frag_color = vec4(color, alpha);
}
