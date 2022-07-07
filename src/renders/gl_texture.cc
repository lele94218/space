#include "gl_texture.h"

#include <glad/glad.h>
#include <glog/logging.h>
#include <stab/stab_image.h>

GLTexture::GLTexture(const Material& material) {
  texture_id_ = TextureFromFile(material.map_texture_path);
  // type_ = type_name;
  path_ = material.map_texture_path;
}

unsigned int GLTexture::TextureFromFile(const std::string& file_path, bool gamma) {
  unsigned int texture_id;
  glGenTextures(1, &texture_id);

  int width, height, num_channels;
  unsigned char* data = stbi_load(file_path.c_str(), &width, &height, &num_channels, 0);
  if (data) {
    GLenum format;
    if (num_channels == 1)
      format = GL_RED;
    else if (num_channels == 3)
      format = GL_RGB;
    else if (num_channels == 4)
      format = GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
  } else {
    LOG(ERROR) << "Texture failed to load at path: " << file_path;
    stbi_image_free(data);
  }

  return texture_id;
}
