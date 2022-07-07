#pragma once

#include <assimp/scene.h>

#include <memory>
#include <string>

#include "../core/mesh_object.h"
#include "../core/object_3d.h"

class ObjLoader {
 public:
  ObjLoader(const std::string& file_path);

  void Load();
  void ProcessNode(aiNode* node, const aiScene* scene);

  std::unique_ptr<Object3D> object_3d_;

 private:
  std::unique_ptr<MeshObject> ProcessMesh(aiMesh* mesh, const aiScene* scene);

  std::string root_dir_;
  std::string file_name_;
};
