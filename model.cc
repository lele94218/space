#include "model.h"

#include <glad/glad.h>
#include <glog/logging.h>
#include <stab/stab_image.h>

namespace {

unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma = false) {
  std::string filename(path);
  filename = directory + '/' + filename;

  unsigned int texture_id;
  glGenTextures(1, &texture_id);

  int width, height, num_channels;
  unsigned char* data = stbi_load(filename.c_str(), &width, &height, &num_channels, 0);
  if (data) {
    GLenum format;
    if (num_channels == 1)
      format = GL_RED;
    else if (num_channels == 3)
      format = GL_RGB;
    else if (num_channels == 4)
      format = GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
  } else {
    LOG(ERROR) << "Texture failed to load at path: " << path;
    stbi_image_free(data);
  }

  return texture_id;
}

}  // namespace

void Model::Draw(Shader& shader) {
  for (unsigned int i = 0; i < meshes_.size(); i++) {
    meshes_[i].Draw(shader);
  }
}

void Model::LoadModel(const std::string& path) {
  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);
  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
    LOG(ERROR) << "ERROR::ASSIMP::" << importer.GetErrorString();
    return;
  }
  directory_ = path.substr(0, path.find_last_of('/'));
  ProcessNode(scene->mRootNode, scene);
}

void Model::ProcessNode(aiNode* node, const aiScene* scene) {
  // process all the node's meshes (if any)
  for (unsigned int i = 0; i < node->mNumMeshes; i++) {
    aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
    meshes_.push_back(ProcessMesh(mesh, scene));
  }
  // then do the same for each of its children
  for (unsigned int i = 0; i < node->mNumChildren; i++) {
    ProcessNode(node->mChildren[i], scene);
  }
}

Mesh Model::ProcessMesh(aiMesh* mesh, const aiScene* scene) {
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  std::vector<Texture> textures;

  for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
    Vertex vertex;
    // process vertex positions, normals and texture coordinates
    {
      glm::vec3 vec;
      vec.x = mesh->mVertices[i].x;
      vec.y = mesh->mVertices[i].y;
      vec.z = mesh->mVertices[i].z;
      vertex.position = vec;
    }
    {
      glm::vec3 vec;
      vec.x = mesh->mNormals[i].x;
      vec.y = mesh->mNormals[i].y;
      vec.z = mesh->mNormals[i].z;
      vertex.normal = vec;
    }
    {
      if (mesh->mTextureCoords[0]) {
        // The mesh contains texture coordinates
        glm::vec2 vec;
        vec.x = mesh->mTextureCoords[0][i].x;
        vec.y = mesh->mTextureCoords[0][i].y;
        vertex.tex_coords = vec;
      } else {
        vertex.tex_coords = glm::vec2(0.0f, 0.0f);
      }
    }
    vertices.push_back(vertex);
  }
  // process indices
  for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
    const aiFace& face = mesh->mFaces[i];
    for (unsigned int j = 0; j < face.mNumIndices; j++) {
      indices.push_back(face.mIndices[j]);
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

  return Mesh(vertices, indices, textures);
}

std::vector<Texture> Model::LoadMaterialTextures(aiMaterial* mat,
                                                 aiTextureType type,
                                                 const std::string& type_name) {
  std::vector<Texture> textures;
  for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
    aiString str;
    mat->GetTexture(type, i, &str);
    bool skip = false;
    for (unsigned int j = 0; j < textures_loaded_.size(); j++) {
      if (std::strcmp(textures_loaded_[j].path.data(), str.C_Str()) == 0) {
        textures.push_back(textures_loaded_[j]);
        skip = true;
        break;
      }
    }
    if (!skip) {
      // if texture hasn't been loaded already, load it
      Texture texture;
      texture.id = TextureFromFile(str.C_Str(), directory_);
      texture.type = type_name;
      texture.path = str.C_Str();
      textures.push_back(texture);
      textures_loaded_.push_back(texture);
    }
  }
  return textures;
}
