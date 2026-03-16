#pragma once

#include <string>
#include "../core/material.h"

class GLTexture {
 public:
  // Construct from a Material.
  // If material.gl_albedo != 0, GL IDs are used directly (GLTFLoader path).
  // Otherwise, textures are loaded from file paths (ObjLoader path).
  explicit GLTexture(const Material& material);

  unsigned int diffuse_id()            const { return diffuse_id_; }
  unsigned int specular_id()           const { return specular_id_; }
  unsigned int normal_id()             const { return normal_id_; }
  unsigned int roughness_id()          const { return roughness_id_; }
  unsigned int ao_id()                 const { return ao_id_; }
  unsigned int metallic_roughness_id() const { return metallic_roughness_id_; }
  unsigned int occlusion_id()          const { return ao_id_; }
  unsigned int emissive_id()           const { return emissive_id_; }

  // KHR_materials_clearcoat textures
  unsigned int clearcoat_normal_tex_id() const { return clearcoat_normal_tex_id_; }

  bool has_specular()          const { return specular_id_          != 0; }
  bool has_normal()            const { return normal_id_            != 0; }
  bool has_roughness()         const { return roughness_id_         != 0; }
  bool has_ao()                const { return ao_id_                != 0; }
  bool has_metallic_roughness() const { return metallic_roughness_id_ != 0; }
  bool has_emissive()          const { return emissive_id_          != 0; }
  bool has_clearcoat_normal()  const { return clearcoat_normal_tex_id_ != 0; }

  // Clear the global file-texture cache (call when switching models)
  static void ClearFileCache();

 private:
  unsigned int diffuse_id_              = 0;
  unsigned int specular_id_             = 0;
  unsigned int normal_id_               = 0;
  unsigned int roughness_id_            = 0;
  unsigned int ao_id_                   = 0;
  unsigned int metallic_roughness_id_   = 0;
  unsigned int emissive_id_             = 0;
  unsigned int clearcoat_normal_tex_id_ = 0;
  std::string  path_;
};
