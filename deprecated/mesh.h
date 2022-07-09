#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

#include "shader.h"
#include "texture.h"

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
