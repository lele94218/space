#pragma once

#include <glad/glad.h>

#include "../core/geometry.h"

class GLBindingState {
 public:
  GLBindingState(const Geometry& geometry);

  unsigned int vao() const { return vao_; }

 private:
  static unsigned int MakeBuffer(unsigned int target,
                                 unsigned int buffer_size,
                                 const void* buffer_data);

  unsigned int vao_;
  unsigned int vbo_;
  unsigned int veo_;
};
