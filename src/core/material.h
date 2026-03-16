#pragma once

#include <string>

class Material {
 public:
  Material();

  int id;
  bool flip_y = true;
  std::string map_texture_path;           // diffuse
  std::string metalness_map_texture_path; // specular
  std::string normal_map_texture_path;    // normal map
  std::string shader_name;

 private:
  static int unique_id_;
};
