#pragma once

#include "object_3d.h"

class SceneObject : public Object3D {
 public:
  SceneObject() : Object3D("") {}
  virtual ~SceneObject() = default;
};
