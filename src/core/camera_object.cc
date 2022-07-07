#include "camera_object.h"

#include <glm/gtc/matrix_transform.hpp>

CameraObject::CameraObject(float fov, float aspect, float near, float far)
    : Object3D(""), fov_(fov), aspect_(aspect), near_(near), far_(far) {
  UpdateProjectionMatrix();
}

void CameraObject::UpdateProjectionMatrix() {
  projection_matrix_ = glm::perspective(fov_, aspect_, near_, far_);
}

void CameraObject::LookAt(const glm::vec3& target) {
  view_matrix_ = glm::lookAt(position_, target, up_);
}
