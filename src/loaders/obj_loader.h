#pragma once

#include <assimp/scene.h>

#include <memory>
#include <string>

#include "../core/mesh_object.h"
#include "../core/object_3d.h"

class ObjLoader {
 public:
  ObjLoader(const std::string& path);
  void Load();

 private:
  void ProcessNode(aiNode* node, const aiScene* scene);
  MeshObject ProcessMesh(aiMesh* mesh, const aiScene* scene);

  std::string root_dir_;
  std::unique_ptr<Object3D> object_3d_;
};
