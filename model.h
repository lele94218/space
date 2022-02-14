#pragma once

#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#include <string>
#include <vector>

#include "mesh.h"
#include "shader.h"
#include "texture.h"

class Model {
 public:
  Model(const std::string& path) { LoadModel(path); }
  void Draw(Shader& shader);

 private:
  void LoadModel(const std::string& path);
  void ProcessNode(aiNode* node, const aiScene* scene);
  Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);
  std::vector<Texture> LoadMaterialTextures(aiMaterial* mat,
                                            aiTextureType type,
                                            const std::string& type_name);

  // model data
  std::vector<Mesh> meshes_;
  std::vector<Texture> textures_loaded_;
  std::string directory_;
};
