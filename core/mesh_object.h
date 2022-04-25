#pragma once

struct Geometry {
  std::vector<glm::vec3> position;
  std::vector<glm::vec3> normal;
  std::vector<unsigned int> index;
  std::vector<glm::vec2> uv;
};

struct Material {
  bool flip_y = true;
  std::string texture_path;
};

class MeshObject : public Object3D {
 public:
  MeshObject(const Geometry& geometry, const Material& material);

 private:
  Geometry geometry_;
  Material material_;
};
