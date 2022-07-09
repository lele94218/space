#pragma once

#include <string>

class Material {
 public:
  Material();

  int id;
  bool flip_y = true;
  std::string map_texture_path;
  std::string metalness_map_texture_path;
  std::string shader_name;

 private:
  static int unique_id_;
};
