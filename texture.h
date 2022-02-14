#pragma once

#include <string>

#include "texture.h"

struct Texture {
 public:
  Texture(const std::string& filename, const std::string& directory, const std::string& type_name);
  unsigned int id;
  std::string type;
  std::string path;

 private:
  unsigned int TextureFromFile(const std::string& file_path, bool gamma = false);
};
