#include "obj_loader.h"

#include <assimp/GltfMaterial.h>
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
  // glTF/GLB uses top-left UV origin like OpenGL — FlipUVs would break it.
  // .obj/.fbx typically need FlipUVs to display correctly in OpenGL.
  const bool needs_flip_uvs = (file_name_.rfind(".glb")  == std::string::npos &&
                                file_name_.rfind(".gltf") == std::string::npos);
  unsigned int flags = aiProcess_Triangulate | aiProcess_CalcTangentSpace;
  if (needs_flip_uvs) flags |= aiProcess_FlipUVs;

  const aiScene* scene = importer.ReadFile(file_path, flags);
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
        vec.x = mesh->mTextureCoords[0][i].x;
        vec.y = mesh->mTextureCoords[0][i].y;
      }
      vertex.uv = vec;
    }
    {
      glm::vec3 vec(0.0f, 0.0f, 0.0f);
      if (mesh->HasTangentsAndBitangents()) {
        vec.x = mesh->mTangents[i].x;
        vec.y = mesh->mTangents[i].y;
        vec.z = mesh->mTangents[i].z;
      }
      vertex.tangent = vec;
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

    // Helper lambda: resolve file-based texture path (.obj only)
    auto resolve = [&](aiTextureType type, std::string& out_path) {
      if (!ai_material->GetTextureCount(type)) return;
      ai_material->GetTexture(type, 0, &ai_str);
      std::string raw = ai_str.C_Str();
      if (!raw.empty() && raw[0] != '*')
        out_path = root_dir_ + "/" + raw;
    };

    resolve(aiTextureType_DIFFUSE,           material.map_texture_path);
    resolve(aiTextureType_SPECULAR,          material.metalness_map_texture_path);
    resolve(aiTextureType_NORMALS,           material.normal_map_texture_path);
    if (material.normal_map_texture_path.empty())
      resolve(aiTextureType_HEIGHT,          material.normal_map_texture_path);
    resolve(aiTextureType_DIFFUSE_ROUGHNESS, material.roughness_map_texture_path);
    if (material.roughness_map_texture_path.empty())
      resolve(aiTextureType_SHININESS,       material.roughness_map_texture_path);
    resolve(aiTextureType_AMBIENT_OCCLUSION, material.ao_map_texture_path);
    if (material.ao_map_texture_path.empty())
      resolve(aiTextureType_AMBIENT,         material.ao_map_texture_path);

    // Read alpha mode (glTF)
    {
      aiString alpha_mode_str;
      if (ai_material->Get(AI_MATKEY_GLTF_ALPHAMODE, alpha_mode_str) == AI_SUCCESS) {
        std::string mode = alpha_mode_str.C_Str();
        if (mode == "BLEND") material.alpha_mode = 2;
        else if (mode == "MASK") material.alpha_mode = 1;
        else material.alpha_mode = 0;
      }
      float cutoff = 0.5f;
      if (ai_material->Get(AI_MATKEY_GLTF_ALPHACUTOFF, cutoff) == AI_SUCCESS)
        material.alpha_cutoff = cutoff;
    }

    // Read base color factor (glTF PBR)
    aiColor4D base_color(1.f, 1.f, 1.f, 1.f);
    if (ai_material->Get(AI_MATKEY_BASE_COLOR, base_color) == AI_SUCCESS) {
      material.base_color[0] = base_color.r;
      material.base_color[1] = base_color.g;
      material.base_color[2] = base_color.b;
      material.base_color[3] = base_color.a;
    } else if (ai_material->Get(AI_MATKEY_COLOR_DIFFUSE, base_color) == AI_SUCCESS) {
      material.base_color[0] = base_color.r;
      material.base_color[1] = base_color.g;
      material.base_color[2] = base_color.b;
    }

    material.shader_name = "model_shader";
    LOG(INFO) << "Material[" << material.id << "] "
              << " diffuse=" << material.map_texture_path
              << " normal=" << material.normal_map_texture_path
              << " alpha=" << material.alpha_mode
              << " base_color=[" << material.base_color[0] << ","
              << material.base_color[1] << "," << material.base_color[2] << "]";
  }
  return std::make_unique<MeshObject>(geometry, material);
}
