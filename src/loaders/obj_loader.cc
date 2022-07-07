#include "obj_loader.h"

#include <assimp/postprocess.h>
#include <glog/logging.h>

#include <assimp/Importer.hpp>

ObjLoader::ObjLoader(const std::string& file_path) {
  object_3d_ = std::make_unique<Object3D>("");
  root_dir_ = file_path.substr(0, file_path.find_last_of('/'));
  file_name_ = file_path.substr(file_path.find_last_of('/') + 1, file_path.length());
}

void ObjLoader::Load() {
  Assimp::Importer importer;
  const std::string file_path = root_dir_ + "/" + file_name_;
  const aiScene* scene = importer.ReadFile(file_path, aiProcess_Triangulate | aiProcess_FlipUVs);
  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
    LOG(ERROR) << "ERROR::ASSIMP::" << importer.GetErrorString();
    return;
  }
  ProcessNode(scene->mRootNode, scene);
}

void ObjLoader::ProcessNode(aiNode* node, const aiScene* scene) {
  // process all the node's meshes (if any)
  for (unsigned int i = 0; i < node->mNumMeshes; i++) {
    aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
    auto mesh_object = ProcessMesh(mesh, scene);
    object_3d_->Add(std::move(mesh_object));
  }
  // then do the same for each of its children
  for (unsigned int i = 0; i < node->mNumChildren; i++) {
    ProcessNode(node->mChildren[i], scene);
  }
}

std::unique_ptr<MeshObject> ObjLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene) {
  Geometry geometry;
  Material material;

  for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
    // process vertex positions, normals and texture coordinates
    Vertex vertex;
    {
      glm::vec3 vec;
      vec.x = mesh->mVertices[i].x;
      vec.y = mesh->mVertices[i].y;
      vec.z = mesh->mVertices[i].z;
      vertex.position = vec;
    }
    {
      glm::vec3 vec(0.0f, 0.0f, 0.0f);
      if (mesh->HasNormals()) {
        vec.x = mesh->mNormals[i].x;
        vec.y = mesh->mNormals[i].y;
        vec.z = mesh->mNormals[i].z;
      }
      vertex.normal = vec;
    }
    {
      glm::vec2 vec(0.0f, 0.0f);
      if (mesh->mTextureCoords[0]) {
        // The mesh contains texture coordinates
        vec.x = mesh->mTextureCoords[0][i].x;
        vec.y = mesh->mTextureCoords[0][i].y;
      }
      vertex.uv = vec;
    }
    geometry.vertices.push_back(vertex);
  }
  // process indices
  for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
    const aiFace& face = mesh->mFaces[i];
    for (unsigned int j = 0; j < face.mNumIndices; j++) {
      geometry.index.push_back(face.mIndices[j]);
    }
  }
  // process material
  if (mesh->mMaterialIndex >= 0) {
    aiMaterial* ai_material = scene->mMaterials[mesh->mMaterialIndex];
    aiString ai_str;
    if (ai_material->GetTextureCount(aiTextureType_DIFFUSE)) {
      LOG(ERROR) << "no";
      ai_material->GetTexture(aiTextureType_DIFFUSE, 0, &ai_str);
      material.map_texture_path = root_dir_ + "/" + ai_str.C_Str();
    }
    if (ai_material->GetTextureCount(aiTextureType_SPECULAR)) {
      ai_material->GetTexture(aiTextureType_SPECULAR, 0, &ai_str);
      material.metalness_map_texture_path = root_dir_ + "/" + ai_str.C_Str();
    }
  }
  return std::make_unique<MeshObject>(geometry, material);
}
