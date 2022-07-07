#pragma once

#include <string>

#include "geometry.h"
#include "material.h"
#include "object_3d.h"

class MeshObject : public Object3D {
 public:
  MeshObject(const Geometry& geometry, const Material& material);
  virtual ~MeshObject() = default;

  const Geometry& geometry() const { return geometry_; }
  const Material& material() const { return material_; }

 private:
  Geometry geometry_;
  Material material_;
};
