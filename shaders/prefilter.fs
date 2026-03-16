#version 330 core
out vec4 frag_color;

in vec3 local_pos;

uniform samplerCube environment_map;
uniform float roughness;

const float PI = 3.14159265359;

// GGX importance sampling
float DistributionGGX(vec3 N, vec3 H, float roughness) {
  float a  = roughness * roughness;
  float a2 = a * a;
  float d  = max(dot(N, H), 0.0);
  float denom = d * d * (a2 - 1.0) + 1.0;
  return a2 / (PI * denom * denom);
}

// Low-discrepancy Hammersley sequence
float RadicalInverse_VdC(uint bits) {
  bits = (bits << 16u) | (bits >> 16u);
  bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
  bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
  bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
  bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
  return float(bits) * 2.3283064365386963e-10;
}

vec2 Hammersley(uint i, uint N) {
  return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
  float a = roughness * roughness;

  float phi      = 2.0 * PI * Xi.x;
  float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
  float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

  // Spherical to Cartesian (tangent space)
  vec3 H = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

  // Tangent to world space
  vec3 up    = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
  vec3 right = normalize(cross(up, N));
  up         = cross(N, right);

  return normalize(right * H.x + up * H.y + N * H.z);
}

void main() {
  vec3 N = normalize(local_pos);
  // Assume V = R = N (isotropic view assumption for prefilter)
  vec3 R = N;
  vec3 V = R;

  const uint SAMPLE_COUNT = 1024u;
  vec3  prefiltered_color = vec3(0.0);
  float total_weight      = 0.0;

  for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
    vec2 Xi = Hammersley(i, SAMPLE_COUNT);
    vec3 H  = ImportanceSampleGGX(Xi, N, roughness);
    vec3 L  = normalize(2.0 * dot(V, H) * H - V);

    float NdotL = max(dot(N, L), 0.0);
    if (NdotL > 0.0) {
      // Sample from the environment's mip level based on roughness/pdf
      float D       = DistributionGGX(N, H, roughness);
      float NdotH   = max(dot(N, H), 0.0);
      float HdotV   = max(dot(H, V), 0.0);
      float pdf     = D * NdotH / (4.0 * HdotV) + 0.0001;

      float resolution  = 512.0; // resolution of the env cubemap face
      float sa_texel    = 4.0 * PI / (6.0 * resolution * resolution);
      float sa_sample   = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);
      float mip_level   = roughness == 0.0 ? 0.0 : 0.5 * log2(sa_sample / sa_texel);

      prefiltered_color += textureLod(environment_map, L, mip_level).rgb * NdotL;
      total_weight      += NdotL;
    }
  }

  prefiltered_color = prefiltered_color / total_weight;
  frag_color = vec4(prefiltered_color, 1.0);
}
