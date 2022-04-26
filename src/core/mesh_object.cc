#include "mesh_object.h"

MeshObject::MeshObject(const Geometry& geometry, const Material& material)
    : Object3D(""), geometry_(geometry), material_(material) {}
