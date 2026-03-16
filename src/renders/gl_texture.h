#pragma once

#include <string>

#include "../core/material.h"

class GLTexture {
 public:
  GLTexture(const Material& material);

  unsigned int diffuse_id()   const { return diffuse_id_; }
  unsigned int specular_id()  const { return specular_id_; }
  unsigned int normal_id()    const { return normal_id_; }
  unsigned int roughness_id() const { return roughness_id_; }
  unsigned int ao_id()        const { return ao_id_; }

  bool has_specular()  const { return specular_id_  != 0; }
  bool has_normal()    const { return normal_id_    != 0; }
  bool has_roughness() const { return roughness_id_ != 0; }
  bool has_ao()        const { return ao_id_        != 0; }

  // Clear the global file texture cache (call when switching models)
  static void ClearFileCache();

 private:

  unsigned int diffuse_id_   = 0;
  unsigned int specular_id_  = 0;
  unsigned int normal_id_    = 0;
  unsigned int roughness_id_ = 0;
  unsigned int ao_id_        = 0;
  std::string path_;
};
