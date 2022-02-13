#include "mesh.h"

#include <glad/glad.h>

Mesh::Mesh(const std::vector<Vertex>& vertices,
           const std::vector<unsigned int>& indices,
           const std::vector<Texture>& textures) {
  vertices_ = vertices;
  indices_ = indices;
  textures_ = textures;
  // now that we have all the required data, set the vertex buffers and its attribute pointers.
  SetupMesh();
}

void Mesh::SetupMesh() {
  // create buffers/arrays
  glGenVertexArrays(1, &VAO_);
  glGenBuffers(1, &VBO_);
  glGenBuffers(1, &EBO_);

  glBindVertexArray(VAO_);
  // load data into vertex buffers
  glBindBuffer(GL_ARRAY_BUFFER, VBO_);
  // A great thing about structs is that their memory layout is sequential for all its items.
  // The effect is that we can simply pass a pointer to the struct and it translates perfectly to
  // a glm::vec3/2 array which again translates to 3/2 floats which translates to a byte array.
  glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(Vertex), &vertices_[0], GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               indices_.size() * sizeof(unsigned int),
               &indices_[0],
               GL_STATIC_DRAW);

  // set the vertex attribute pointers
  // vertex Positions
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
  // vertex normals
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
  // vertex texture coords
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(
      2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tex_coords));
  // Unbind all
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void Mesh::Draw(const Shader& shader) const {
  // bind appropriate textures
  unsigned int diffuseNr = 1;
  unsigned int specularNr = 1;
  unsigned int normalNr = 1;
  unsigned int heightNr = 1;
  for (unsigned int i = 0; i < textures_.size(); i++) {
    // active proper texture unit before binding
    glActiveTexture(GL_TEXTURE0 + i);
    // retrieve texture number (the N in diffuse_textureN)
    std::string number;
    const std::string& name = textures_[i].type;
    if (name == "texture_diffuse")
      number = std::to_string(diffuseNr++);
    else if (name == "texture_specular")
      number = std::to_string(specularNr++);  // transfer unsigned int to string
    else if (name == "texture_normal")
      number = std::to_string(normalNr++);  // transfer unsigned int to string
    else if (name == "texture_height")
      number = std::to_string(heightNr++);  // transfer unsigned int to string

    // now set the sampler to the correct texture unit
    shader.setInt(name + number, i);
    // and finally bind the texture
    glBindTexture(GL_TEXTURE_2D, textures_[i].id);
  }

  // draw mesh
  glBindVertexArray(VAO_);
  glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices_.size()), GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);

  // always good practice to set everything back to defaults once configured.
  glActiveTexture(GL_TEXTURE0);
}
