#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

// Defines several possible options for camera movement. Used as abstraction to stay away from
// window-system specific input methods
enum class CameraMovement {
  FORWARD = 1,
  BACKWARD = 2,
  LEFT = 3,
  RIGHT = 4,
};

// An abstract camera class that processes input and calculates the corresponding Euler Angles,
// Vectors and Matrices for use in OpenGL
class Camera {
 public:
  // constructor with vectors
  Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
         glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
         float yaw = -90.0f,
         float pitch = 0.0f);
  // constructor with scalar values
  Camera(
      float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch);

  // returns the view matrix calculated using Euler Angles and the LookAt Matrix
  glm::mat4 GetViewMatrix() const;

  // processes input received from any keyboard-like input system. Accepts input parameter in the
  // form of camera defined ENUM (to abstract it from windowing systems)
  void ProcessKeyboard(CameraMovement direction, float deltaTime);

  // processes input received from a mouse input system. Expects the offset value in both the x and
  // y direction.
  void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);

  // processes input received from a mouse scroll-wheel event. Only requires input on the vertical
  // wheel-axis
  void ProcessMouseScroll(float yoffset);

  float zoom() const { return zoom_; }

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
