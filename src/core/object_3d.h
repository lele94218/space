#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

class Object3D {
 public:
  Object3D(const std::string& name);
  virtual ~Object3D() = default;

  void Add(std::unique_ptr<Object3D> object_3d);

  const std::string& name() const { return name_; }
  int size() const { return children_.size(); }
  const std::vector<std::unique_ptr<Object3D>>& children() { return children_; }
  Object3D* get(int index) const;

 protected:
  std::string name_;
  glm::vec3 position_;
  glm::vec3 up_ = glm::vec3(0.0f, 1.0f, 0.0f);

  std::vector<std::unique_ptr<Object3D>> children_;
};
