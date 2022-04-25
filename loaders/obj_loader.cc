#include "loaders/obj_loader.h"

#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>

ObjLoader::ObjLoader(const std::string& path) {
  root_dir_ = path.substr(0, path.find_last_of('/'));
}

void ObjLoader::Load() {
  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);
  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
    LOG(ERROR) << "ERROR::ASSIMP::" << importer.GetErrorString();
    return;
  }
  ProcessNode(scene->mRootNode, scene);
}

void ObjLoader::ProcessNode(aiNode* node, aiScene* scene) {
  // process all the node's meshes (if any)
  for (unsigned int i = 0; i < node->mNumMeshes; i++) {
    aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
    MeshObject mesh_object = ProcessMesh(mesh, scene);
    object_3d_.Add(std::make_unique<MeshObject>(mesh_object));
  }
  // then do the same for each of its children
  for (unsigned int i = 0; i < node->mNumChildren; i++) {
    ProcessNode(node->mChildren[i], scene);
  }
}

MeshObject Model::ProcessMesh(aiMesh* mesh, aiScene* scene) {
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  std::vector<Texture> textures;
  Geometry geometry;
  Material material;

  for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
    Vertex vertex;
    // process vertex positions, normals and texture coordinates
    {
      glm::vec3 vec;
      vec.x = mesh->mVertices[i].x;
      vec.y = mesh->mVertices[i].y;
      vec.z = mesh->mVertices[i].z;
      geometry.position.push_back(vec);
    }
    {
      glm::vec3 vec(0.0f, 0.0f, 0.0f);
      if (mesh->HasNormals()) {
        vec.x = mesh->mNormals[i].x;
        vec.y = mesh->mNormals[i].y;
        vec.z = mesh->mNormals[i].z;
      }
      geometry.normal.push_back(vec);
    }
    {
      glm::vec2 vec(0.0f, 0.0f);
      if (mesh->mTextureCoords[0]) {
        // The mesh contains texture coordinates
        vec.x = mesh->mTextureCoords[0][i].x;
        vec.y = mesh->mTextureCoords[0][i].y;
      }
      geometry.uv.push_back(vec);
    }
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
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    std::vector<Texture> diffuseMaps =
        LoadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    std::vector<Texture> specularMaps =
        LoadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
  }
  return Mesh(geometry, material);
}
