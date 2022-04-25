#pragma once

#include <string>
#include <vector>

class Object3D {
 public:
  Object3D(const std::string& name);

  void Add(std::unique<Object3D>& object_3d);
  const std::string& name() const { return name_; }

 protected:
  std::string name_;
  vector<std::unique<Object3D>> children_;
};
