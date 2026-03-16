#pragma once

#include <string>

// GLIBL: Image-Based Lighting precomputation.
//
// Loads an HDR equirectangular environment map and converts it to:
//   - env_cubemap_:    raw environment cubemap (HDR, RGB16F)
//   - irradiance_map_: diffuse irradiance integral over hemisphere (low-res)
//   - prefilter_map_:  specular split-sum prefiltered cubemap (mip-mapped)
//   - brdf_lut_:       split-sum BRDF look-up texture (scale/bias for F0)
//
// Usage:
//   GLIBL ibl;
//   ibl.Load("assets/env/neutral.hdr");
//   if (ibl.loaded()) { ... bind textures ... }
class GLIBL {
 public:
  GLIBL() = default;
  ~GLIBL();

  // Non-copyable (owns GL resources)
  GLIBL(const GLIBL&)            = delete;
  GLIBL& operator=(const GLIBL&) = delete;

  // Load HDR equirectangular and precompute all IBL maps.
  // Returns true on success.
  bool Load(const std::string& hdr_path);

  unsigned int env_cubemap()    const { return env_cubemap_; }
  unsigned int irradiance_map() const { return irradiance_map_; }
  unsigned int prefilter_map()  const { return prefilter_map_; }
  unsigned int brdf_lut()       const { return brdf_lut_; }
  bool         loaded()         const { return loaded_; }

 private:
  // Render equirectangular HDR texture into a new cubemap of given face size.
  unsigned int CreateCubemapFromEquirect(unsigned int hdr_tex, int size);

  // Convolve environment cubemap for diffuse irradiance.
  unsigned int CreateIrradianceMap(unsigned int env_cubemap, int size = 32);

  // Build split-sum specular prefiltered cubemap (5 roughness mip levels).
  unsigned int CreatePrefilterMap(unsigned int env_cubemap, int size = 128);

  // Compute the BRDF integration LUT (NdotV x roughness → scale/bias).
  unsigned int CreateBRDFLUT(int size = 512);

  // Render a unit cube — used for all cubemap capture passes.
  void RenderCube();

  // Render a full-screen quad — used for the BRDF LUT pass.
  void RenderQuad();

  unsigned int cube_vao_ = 0;
  unsigned int cube_vbo_ = 0;
  unsigned int quad_vao_ = 0;
  unsigned int quad_vbo_ = 0;

  unsigned int env_cubemap_    = 0;
  unsigned int irradiance_map_ = 0;
  unsigned int prefilter_map_  = 0;
  unsigned int brdf_lut_       = 0;
  bool         loaded_         = false;
};
