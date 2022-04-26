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
  children_.push_back(std::move(object_3d));
}
