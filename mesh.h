#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader.h"

struct Texture {
  unsigned int id;
  std::string type;
  std::string path;
};

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 tex_coords;
};

class Mesh {
 public:
  Mesh(const std::vector<Vertex>& vertices,
       const std::vector<unsigned int>& indices,
       const std::vector<Texture>& textures);

  // render the mesh
  void Draw(const Shader& shader) const;

 private:
  // initializes all the buffer objects/arrays
  void SetupMesh();

  std::vector<Vertex> vertices_;
  std::vector<unsigned int> indices_;
  std::vector<Texture> textures_;

  // render data
  unsigned int VAO_, VBO_, EBO_;
};
