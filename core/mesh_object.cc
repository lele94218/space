#include "core/mesh_object.h"

MeshObject::MeshObject(const Geometry& geometry, const Material& material)
    : geometry_(geometry), material_(material) {}
