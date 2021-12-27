#pragma once

#include <string>

enum class ImageFormat {
  JPEG = 1,
  PNG = 2,
};

class Texture {
 public:
  unsigned int ID;

  Texture(const std::string& image_path, ImageFormat format);
  void use(int target_id) const;
};
