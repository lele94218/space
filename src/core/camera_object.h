#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

#include "object_3d.h"

// Defines several possible options for camera movement. Used as abstraction to stay away from
// window-system specific input methods
enum class CameraMovement {
  FORWARD = 1,
  BACKWARD = 2,
  LEFT = 3,
  RIGHT = 4,
};

class CameraObject : public Object3D {
 public:
  // constructor with vectors
  CameraObject(glm::vec3 position = glm::vec3(0.0f, 0.0f, 10.0f),
               glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
               float yaw = -90.0f,
               float pitch = 0.0f);
  virtual ~CameraObject() = default;

  // returns the view matrix calculated using Euler Angles and the LookAt Matrix
  glm::mat4 GetViewMatrix() const;

  // processes input received from any keyboard-like input system. Accepts input parameter in the
  // form of camera defined ENUM (to abstract it from windowing systems)
  void ProcessKeyboard(CameraMovement direction, float deltaTime);

  // processes input received from a mouse input system. Expects the offset value in both the x and
  // y direction.
  void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

  // processes input received from a mouse scroll-wheel event. Only requires input on the vertical
  // wheel-axis
  void ProcessMouseScroll(float offset_y);

  float zoom() const { return zoom_; }
  glm::vec3 position() const { return position_; }

 private:
  // calculates the front vector from the Camera's (updated) Euler Angles
  void UpdateCameraVectors();

  // camera Attributes
  glm::vec3 position_;
  glm::vec3 front_;
  glm::vec3 up_;
  glm::vec3 right_;
  glm::vec3 worldUp_;
  // euler Angles
  float yaw_;
  float pitch_;
  // camera options
  float movementSpeed_;
  float mouseSensitivity_;
  float zoom_;
};
