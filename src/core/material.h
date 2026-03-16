#pragma once

#include <string>

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

 private:
  static int unique_id_;
};
