#include "gl_texture.h"

#include <glad/glad.h>
#include <glog/logging.h>
#include <stab/stab_image.h>
#include <unordered_map>
#include <vector>

// Global cache: file path -> GL texture ID (avoids reloading same file for multiple meshes)
static std::unordered_map<std::string, unsigned int> g_file_tex_cache;

void GLTexture::ClearFileCache() {
  g_file_tex_cache.clear();
}

static unsigned int UploadTexture(int width, int height, int num_channels, unsigned char* data) {
  GLenum format = GL_RGB;
  if (num_channels == 1)      format = GL_RED;
  else if (num_channels == 3) format = GL_RGB;
  else if (num_channels == 4) format = GL_RGBA;

  unsigned int id;
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  return id;
}

// Load texture: file path or embedded "*N" data
static unsigned int LoadTex(const std::string& path,
                             const std::unordered_map<std::string, std::vector<unsigned char>>& embedded) {
  if (path.empty()) return 0;

  // Check file-based cache first
  if (path[0] != '*') {
    auto it = g_file_tex_cache.find(path);
    if (it != g_file_tex_cache.end())
      return it->second;
  }

  int width, height, num_channels;
  unsigned char* data = nullptr;

  if (!path.empty() && path[0] == '*') {
    // Embedded texture (GLB)
    auto it = embedded.find(path);
    if (it == embedded.end()) {
      LOG(ERROR) << "Embedded texture not found: " << path;
      return 0;
    }
    data = stbi_load_from_memory(it->second.data(), (int)it->second.size(),
                                 &width, &height, &num_channels, 0);
  } else {
    data = stbi_load(path.c_str(), &width, &height, &num_channels, 0);
  }

  if (!data) {
    LOG(ERROR) << "Texture failed to load: " << path;
    return 0;
  }

  unsigned int id = UploadTexture(width, height, num_channels, data);
  stbi_image_free(data);

  // Cache file-based textures
  if (path[0] != '*')
    g_file_tex_cache[path] = id;

  return id;
}

GLTexture::GLTexture(const Material& material) {
  const auto& emb = material.embedded_textures;
  diffuse_id_   = LoadTex(material.map_texture_path,           emb);
  specular_id_  = LoadTex(material.metalness_map_texture_path, emb);
  normal_id_    = LoadTex(material.normal_map_texture_path,    emb);
  roughness_id_ = LoadTex(material.roughness_map_texture_path, emb);
  ao_id_        = LoadTex(material.ao_map_texture_path,        emb);
  path_ = material.map_texture_path;
}
