#pragma once

#include <string>

#include "core/mesh_object.h"
#include "core/object_3d.h"

class ObjLoader {
 public:
  ObjLoader(const std::string& path);
  void Load();

 private:
  void ProcessNode(aiNode* node, aiScene* scene);
  MeshObject ProcessMesh(aiMesh* mesh, aiScene* scene);

  const std::string root_dir_;
  Object3D object_3d_;
};
