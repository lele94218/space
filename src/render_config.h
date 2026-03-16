#pragma once

// Runtime render pipeline configuration.
// Passed to GLRenderer to control shading mode and effects.
struct RenderConfig {
  // Anti-aliasing
  bool msaa_enabled = true;
  int  msaa_samples = 8;    // options: 1, 2, 4, 8

  // Shading mode
  bool use_pbr = true;      // false = Blinn-Phong, true = PBR (Cook-Torrance)

  // PBR fallback values (used when textures are missing)
  float pbr_metallic  = 0.0f;
  float pbr_roughness = 0.5f;

  // Light
  float light_intensity = 100.0f;
  float light_pos[3]    = {3.0f, 5.0f, 5.0f};

  // IBL
  float ibl_intensity = 1.0f;
};
