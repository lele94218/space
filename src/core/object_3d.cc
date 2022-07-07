#include "object_3d.h"

#include <glog/logging.h>

Object3D::Object3D(const std::string& name) {
  if (name.empty()) {
    name_ = "unknown";
  }
  name_ = name;
}

void Object3D::Add(std::unique_ptr<Object3D> object_3d) {
  CHECK_NOTNULL(object_3d.get());
  children_.emplace_back(std::move(object_3d));
}

Object3D* Object3D::get(int index) const {
  CHECK(index >= 0 && index < size())
      << "Index: " << index << " is out of range of this object's children";
  return children_.at(index).get();
}
