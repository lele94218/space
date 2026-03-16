#pragma once

#include <string>
#include <unordered_map>

#include "gl_binding_state.h"
#include "gl_program.h"
#include "gl_texture.h"

class GLGlobalResources {
 public:
  static GLGlobalResources& GetInstance() {
    static GLGlobalResources instance;
    return instance;
  }
  std::unordered_map<unsigned int, std::unique_ptr<GLBindingState>>& binding_states() {
    return gl_binding_states_;
  }
  std::unordered_map<std::string, std::unique_ptr<GLTexture>>& textures() { return gl_textures_; }
  std::unordered_map<std::string, std::unique_ptr<GLProgram>>& programs() { return gl_programs_; }

  // Clear mesh-specific resources (textures + binding states) when switching models.
  // Shaders are kept since they're reusable.
  void ClearMeshResources() {
    gl_binding_states_.clear();
    gl_textures_.clear();
  }

 private:
  GLGlobalResources() = default;

  std::unordered_map<unsigned int, std::unique_ptr<GLBindingState>> gl_binding_states_;
  std::unordered_map<std::string, std::unique_ptr<GLTexture>> gl_textures_;
  std::unordered_map<std::string, std::unique_ptr<GLProgram>> gl_programs_;
};
