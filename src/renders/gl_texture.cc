#include "gl_texture.h"

#include <glad/glad.h>
#include <glog/logging.h>
#include <stab/stab_image.h>
#include <string>
#include <unordered_map>
#include <vector>

// Global cache: file path -> GL texture ID
static std::unordered_map<std::string, unsigned int> g_file_tex_cache;

void GLTexture::ClearFileCache() {
  g_file_tex_cache.clear();
}

static unsigned int UploadToGL(int width, int height, int channels, const unsigned char* data) {
  GLenum fmt = GL_RGB;
  if (channels == 1)      fmt = GL_RED;
  else if (channels == 4) fmt = GL_RGBA;

  unsigned int id;
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  glTexImage2D(GL_TEXTURE_2D, 0, fmt, width, height, 0, fmt, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  return id;
}

static unsigned int LoadFromFile(const std::string& path) {
  if (path.empty()) return 0;
  auto it = g_file_tex_cache.find(path);
  if (it != g_file_tex_cache.end()) return it->second;

  int w, h, c;
  unsigned char* data = stbi_load(path.c_str(), &w, &h, &c, 0);
  if (!data) { LOG(ERROR) << "Texture failed: " << path; return 0; }
  unsigned int id = UploadToGL(w, h, c, data);
  stbi_image_free(data);
  g_file_tex_cache[path] = id;
  return id;
}

GLTexture::GLTexture(const Material& material) {
  // GLTFLoader path: direct GL IDs already uploaded
  if (material.gl_albedo != 0) {
    diffuse_id_             = material.gl_albedo;
    metallic_roughness_id_  = material.gl_metallic_roughness;
    normal_id_              = material.gl_normal;
    ao_id_                  = material.gl_occlusion;
    emissive_id_            = material.gl_emissive;
    // Also expose metallic_roughness as roughness/specular for legacy code paths
    roughness_id_ = material.gl_metallic_roughness;
    specular_id_  = material.gl_metallic_roughness;
    path_ = "gltf";
    return;
  }

  // ObjLoader path: load from file
  diffuse_id_   = LoadFromFile(material.map_texture_path);
  specular_id_  = LoadFromFile(material.metalness_map_texture_path);
  normal_id_    = LoadFromFile(material.normal_map_texture_path);
  roughness_id_ = LoadFromFile(material.roughness_map_texture_path);
  ao_id_        = LoadFromFile(material.ao_map_texture_path);
  path_ = material.map_texture_path;
}
