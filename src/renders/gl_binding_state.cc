#include "gl_binding_state.h"

#include "../errors/gl_error.h"

#include <glog/logging.h>

GLBindingState::GLBindingState(const Geometry& geometry) {
  // create buffers/arrays
  glGenVertexArrays(1, &vao_);
  glBindVertexArray(vao_);
  vbo_ =
      MakeBuffer(GL_ARRAY_BUFFER, geometry.vertices.size() * sizeof(Vertex), &geometry.vertices[0]);
  veo_ = MakeBuffer(
      GL_ELEMENT_ARRAY_BUFFER, geometry.index.size() * sizeof(unsigned int), &geometry.index[0]);

  GLCheckError();
  // Position
  glEnableVertexAttribArray(0);
  GLCheckError();
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
  GLCheckError();
  // Normal
  glEnableVertexAttribArray(1);
  GLCheckError();
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
  GLCheckError();
  // uv
  glEnableVertexAttribArray(2);
  GLCheckError();
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
  GLCheckError();

  // Unbind all
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  LOG(ERROR) << "state";
}

unsigned int GLBindingState::MakeBuffer(unsigned int target,
                                        unsigned int buffer_size,
                                        const void* buffer_data) {
  unsigned int buffer;
  glGenBuffers(1, &buffer);
  glBindBuffer(target, buffer);
  glBufferData(target, buffer_size, buffer_data, GL_STATIC_DRAW);
  return buffer;
}
