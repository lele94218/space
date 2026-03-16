#pragma once

#include <string>
#include <unordered_map>
#include <vector>

class Material {
 public:
  Material();

  int id;
  bool flip_y = true;
  std::string map_texture_path;           // diffuse / albedo
  std::string metalness_map_texture_path; // specular / metallic
  std::string normal_map_texture_path;    // normal map
  std::string roughness_map_texture_path; // roughness
  std::string ao_map_texture_path;        // ambient occlusion
  std::string shader_name;

  // Embedded texture data (for GLB/GLTF): path "*N" -> raw bytes
  std::unordered_map<std::string, std::vector<unsigned char>> embedded_textures;

  // glTF PBR base color factor (multiplied with albedo texture or used alone)
  float base_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};

 private:
  static int unique_id_;
};
