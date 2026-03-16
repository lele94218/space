#pragma once

#include <string>

class Material {
 public:
  Material();

  int id;
  std::string shader_name;

  // ── File-path based (ObjLoader / Blinn-Phong) ─────────────────────────────
  std::string map_texture_path;           // diffuse / albedo
  std::string metalness_map_texture_path; // specular / metallic
  std::string normal_map_texture_path;    // normal map
  std::string roughness_map_texture_path; // roughness
  std::string ao_map_texture_path;        // ambient occlusion

  // ── Direct GL texture IDs (GLTFLoader — already uploaded to GPU) ──────────
  unsigned int gl_albedo            = 0;
  unsigned int gl_metallic_roughness = 0; // packed: G=roughness, B=metallic
  unsigned int gl_normal            = 0;
  unsigned int gl_occlusion         = 0;
  unsigned int gl_emissive          = 0;

  // ── PBR material factors ──────────────────────────────────────────────────
  float base_color[4]      = {1.0f, 1.0f, 1.0f, 1.0f};
  float metallic_factor    = 1.0f;
  float roughness_factor   = 1.0f;
  float normal_scale       = 1.0f;
  float occlusion_strength = 1.0f;
  float emissive_factor[3] = {0.0f, 0.0f, 0.0f};

  // ── Alpha / culling ───────────────────────────────────────────────────────
  int   alpha_mode   = 0;    // 0=OPAQUE, 1=MASK, 2=BLEND
  float alpha_cutoff = 0.5f;
  bool  double_sided = false;

 private:
  static int unique_id_;
};
