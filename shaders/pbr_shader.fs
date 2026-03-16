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
uniform sampler2D texture_clearcoat_normal; // unit 5

// Texture flags
uniform int use_metallic_rough  = 0;
uniform int use_normal_map      = 0;
uniform int use_occlusion       = 0;
uniform int use_emissive        = 0;
uniform int has_albedo          = 0;
uniform int use_clearcoat_normal = 0;

// KHR_materials_clearcoat
uniform float clearcoat_factor    = 0.0;
uniform float clearcoat_roughness = 0.0;

// KHR_materials_specular
uniform float specular_factor       = 1.0;
uniform vec3  specular_color_factor = vec3(1.0);

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

// IBL
uniform int         use_ibl       = 0;
uniform samplerCube irradiance_map;   // unit 6
uniform samplerCube prefilter_map;    // unit 7
uniform sampler2D   brdf_lut;         // unit 8
uniform float       ibl_intensity = 1.0;

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
  // base_color_factor is already linear (glTF spec); only texture needs sRGB->linear
  vec4 base_tex_linear = vec4(pow(max(albedo_sample.rgb, vec3(0.0)), vec3(2.2)), albedo_sample.a);
  vec4 base = base_tex_linear * base_color_factor;

  // Alpha handling
  float alpha = base.a;
  if (alpha_mode == 1 && alpha < alpha_cutoff) discard;

  // sRGB -> linear for albedo RGB
  vec3 albedo = base.rgb;  // already linear (corrected above)

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
  // KHR_materials_specular: modulate dielectric F0 by specular_factor and color
  vec3 dielectric_F0 = 0.04 * specular_factor * specular_color_factor;
  vec3 F0 = mix(dielectric_F0, albedo, metallic);

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

  // ── KHR_materials_clearcoat layer ────────────────────────────────────────
  if (clearcoat_factor > 0.0) {
    // Clearcoat normal (optional)
    vec3 N_cc;
    if (use_clearcoat_normal == 1) {
      vec3 n_cc = texture(texture_clearcoat_normal, tex_coords).rgb * 2.0 - 1.0;
      N_cc = normalize(TBN * normalize(n_cc));
    } else {
      N_cc = N;
    }
    float cc_rough = max(clearcoat_roughness, 0.04);
    float D_cc  = DistributionGGX(N_cc, H, cc_rough);
    float G_cc  = GeometrySmith(N_cc, V, L, cc_rough);
    float F_cc  = FresnelSchlick(max(dot(H, V), 0.0), vec3(0.04)).x;
    float NdotL_cc = max(dot(N_cc, L), 0.0);
    float cc_spec  = D_cc * G_cc * F_cc / (4.0 * max(dot(N_cc, V), 0.0) * NdotL_cc + 0.0001);
    // Attenuate base layer and add clearcoat contribution
    Lo = Lo * (1.0 - clearcoat_factor * F_cc)
       + vec3(cc_spec) * radiance * NdotL_cc * clearcoat_factor;
  }

  // ── Ambient + AO (IBL or flat) ───────────────────────────────────────────
  vec3 ambient;
  if (use_ibl == 1) {
    // Diffuse IBL: sample irradiance map in normal direction
    vec3 irradiance  = texture(irradiance_map, N).rgb;
    vec3 diffuse_ibl = irradiance * albedo * (1.0 - metallic);

    // Specular IBL: split-sum approximation
    vec3 R = reflect(-V, N);
    const float MAX_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilter_map, R, roughness * MAX_LOD).rgb;
    vec2 brdf_uv = vec2(max(dot(N, V), 0.0), roughness);
    vec2 brdf    = texture(brdf_lut, brdf_uv).rg;
    // Fresnel with roughness for IBL (Lazarov 2012 / Karis 2013)
    vec3 F_ibl = F0 + (max(vec3(1.0 - roughness), F0) - F0)
                 * pow(clamp(1.0 - max(dot(N, V), 0.0), 0.0, 1.0), 5.0);
    vec3 specular_ibl = prefilteredColor * (F_ibl * brdf.x + brdf.y);

    ambient = (diffuse_ibl + specular_ibl) * ao * ibl_intensity;
  } else {
    ambient = vec3(0.03) * albedo * ao;
  }
  vec3 color = ambient + Lo;

  // ── Emissive ─────────────────────────────────────────────────────────────
  if (use_emissive == 1) {
    vec3 emissive = texture(texture_emissive, tex_coords).rgb;
    color += pow(emissive, vec3(2.2)) * emissive_factor;
  } else {
    color += emissive_factor;
  }

  // ── Tone mapping + gamma ─────────────────────────────────────────────────
  // ACESFilmic (Hill 2015) — more filmic contrast than Reinhard
  color *= 0.6;  // exposure pre-scale
  const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
  color = clamp((color*(a*color+b))/(color*(c*color+d)+e), 0.0, 1.0);
  color = pow(color, vec3(1.0 / 2.2));

  frag_color = vec4(color, alpha);
}
