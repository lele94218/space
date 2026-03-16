#pragma once

#include <string>
#include <unordered_map>

#include <glad/glad.h>

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

  // Returns a 1×1 white fallback texture (created lazily).
  // Bind this to any texture unit that has no real texture.
  unsigned int fallback_texture() {
    if (fallback_tex_ == 0) {
      unsigned char white[4] = {255, 255, 255, 255};
      glGenTextures(1, &fallback_tex_);
      glBindTexture(GL_TEXTURE_2D, fallback_tex_);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    return fallback_tex_;
  }

 private:
  GLGlobalResources() = default;
  unsigned int fallback_tex_ = 0;

  std::unordered_map<unsigned int, std::unique_ptr<GLBindingState>> gl_binding_states_;
  std::unordered_map<std::string, std::unique_ptr<GLTexture>> gl_textures_;
  std::unordered_map<std::string, std::unique_ptr<GLProgram>> gl_programs_;
};
