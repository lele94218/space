#include "camera_object.h"

#include <glog/logging.h>

// Default camera values
constexpr float kSpeed = 2.5f;
constexpr float kSensitivity = 0.1f;
constexpr float kZoom = 45.0f;

CameraObject::CameraObject(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : Object3D(""),
      front_(glm::vec3(0.0f, 0.0f, -1.0f)),
      movementSpeed_(kSpeed),
      mouseSensitivity_(kSensitivity),
      zoom_(kZoom) {
  position_ = position;
  worldUp_ = up;
  yaw_ = yaw;
  pitch_ = pitch;
  UpdateCameraVectors();
}

glm::mat4 CameraObject::GetViewMatrix() const {
  return glm::lookAt(position_, position_ + front_, up_);
}

void CameraObject::ProcessKeyboard(CameraMovement direction, float deltaTime) {
  float velocity = movementSpeed_ * deltaTime;
  switch (direction) {
    case CameraMovement::FORWARD:
      position_ += front_ * velocity;
      break;
    case CameraMovement::BACKWARD:
      position_ -= front_ * velocity;
      break;
    case CameraMovement::LEFT:
      position_ -= right_ * velocity;
      break;
    case CameraMovement::RIGHT:
      position_ += right_ * velocity;
      break;
    default:
      LOG(FATAL) << "Unsupported camera movement direction.";
  }
}

void CameraObject::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
  xoffset *= mouseSensitivity_;
  yoffset *= mouseSensitivity_;

  yaw_ += xoffset;
  pitch_ += yoffset;

  // make sure that when pitch is out of bounds, screen doesn't get flipped
  if (constrainPitch) {
    if (pitch_ > 89.0f) pitch_ = 89.0f;
    if (pitch_ < -89.0f) pitch_ = -89.0f;
  }

  // update Front, Right and Up Vectors using the updated Euler angles
  UpdateCameraVectors();
}

void CameraObject::ProcessMouseScroll(float offset_y) {
  zoom_ -= (float)offset_y;
  if (zoom_ < 1.0f) zoom_ = 1.0f;
  if (zoom_ > 45.0f) zoom_ = 45.0f;
}

void CameraObject::UpdateCameraVectors() {
  // calculate the new Front vector
  glm::vec3 next_front;
  next_front.x = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
  next_front.y = sin(glm::radians(pitch_));
  next_front.z = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
  front_ = glm::normalize(next_front);
  // also re-calculate the Right and Up vector
  right_ = glm::normalize(glm::cross(front_, worldUp_));
  // normalize the vectors, because their length gets closer to 0 the more
  // you look up or down which results in slower movement.
  up_ = glm::normalize(glm::cross(right_, front_));
}
