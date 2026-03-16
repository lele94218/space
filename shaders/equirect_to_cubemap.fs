#version 330 core
out vec4 frag_color;

in vec3 local_pos;

uniform sampler2D equirectangular_map;

const vec2 INV_ATAN = vec2(0.1591, 0.3183);

vec2 SampleSphericalMap(vec3 v) {
  vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
  uv *= INV_ATAN;
  uv += 0.5;
  return uv;
}

void main() {
  vec2 uv    = SampleSphericalMap(normalize(local_pos));
  vec3 color = texture(equirectangular_map, uv).rgb;
  frag_color = vec4(color, 1.0);
}
