#pragma once

#include <glm/glm.hpp>

#include "object_3d.h"

class CameraObject : public Object3D {
 public:
  CameraObject(float fov = 50.0f, float aspect = 1.0f, float near = 0.1f, float far = 2000.0f);
  virtual ~CameraObject() = default;

  void LookAt(const glm::vec3& target);

 private:
  void UpdateProjectionMatrix();

  float fov_ = 0.0f;
  float aspect_ = 0.0f;
  float near_ = 0.0f;
  float far_ = 0.0f;

  glm::mat4 projection_matrix_ = glm::mat4(1.0f);
  glm::mat4 view_matrix_ = glm::mat4(1.0f);
};
