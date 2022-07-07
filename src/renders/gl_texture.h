#pragma once

#include <string>

#include "../core/material.h"

class GLTexture {
 public:
  GLTexture(const Material& material);

  unsigned int id() const { return texture_id_; }

 private:
  unsigned int TextureFromFile(const std::string& file_path, bool gamma = false);

  unsigned int texture_id_;
  std::string type_;
  std::string path_;
};
