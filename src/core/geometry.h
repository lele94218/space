#pragma once

#include <glm/glm.hpp>
#include <vector>

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 uv;
};

class Geometry {
 public:
  Geometry();

  int index_size() const { return index.size(); }

  int id;
  std::vector<Vertex> vertices;
  std::vector<unsigned int> index;

 private:
  static int unique_id_;
};
