#include "texture.h"

#include <glad/glad.h>
#include <glog/logging.h>
#include <stab/stab_image.h>

#define GL_ACTIVE_TEXTURE_TARGET(id) glActiveTexture(GL_TEXTURE##id)

Texture::Texture(const std::string& image_path, ImageFormat format) {
  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  // set the texture wrapping/filtering options (on the currently bound texture object)
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // load and generate the texture
  int width, height, nr_channels;
  // TODO: some image is up-side down.
  stbi_set_flip_vertically_on_load(true);
  unsigned char* data = stbi_load(image_path.c_str(), &width, &height, &nr_channels, 0);
  if (data) {
    switch (format) {
      case ImageFormat::PNG:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        break;
      case ImageFormat::JPEG:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        break;
      default:
        LOG(FATAL) << "Unsupported image format: " << static_cast<int>(format);
    }
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
  } else {
    LOG(ERROR) << "Failed to load texutre.";
  }
  ID = texture;
}

void Texture::use(int target_id) const {
  switch(target_id) {
    case 0:
      GL_ACTIVE_TEXTURE_TARGET(0);
      break;
    case 1:
      GL_ACTIVE_TEXTURE_TARGET(1);
      break;
    case 2:
      GL_ACTIVE_TEXTURE_TARGET(2);
      break;
    // case 3-31 should be added manually.
    default:
      LOG(FATAL) << "Unsupported target id: " << target_id;
  }
  glBindTexture(GL_TEXTURE_2D, ID);
}
