#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glog/logging.h>
#include <stab/stab_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include "src/core/camera_object.h"
#include "src/core/scene_object.h"
#include "src/errors/gl_error.h"
#include "src/loaders/obj_loader.h"
#include "src/renders/gl_renderer.h"

float delta_time = 0.0f;  // Time between current frame and last frame
float last_frame = 0.0f;  // Time of last frame
bool first_mouse = true;
// lighting
glm::vec3 light_pos(1.2f, 1.0f, 2.0f);

// camera
CameraObject camera;
// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
float last_x = SCR_WIDTH / 2.0;
float last_y = SCR_HEIGHT / 2.0;
float fov = 45.0f;

// process all input: query GLFW whether relevant keys are pressed/released this frame and react
// accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }
  const float cameraSpeed = 2.5f * delta_time;  // adjust accordingly
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    camera.ProcessKeyboard(CameraMovement::FORWARD, delta_time);
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    camera.ProcessKeyboard(CameraMovement::BACKWARD, delta_time);
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    camera.ProcessKeyboard(CameraMovement::LEFT, delta_time);
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    camera.ProcessKeyboard(CameraMovement::RIGHT, delta_time);
  }
}

void mouseCallback(GLFWwindow* window, double pos_x, double pos_y) {
  if (first_mouse) {
    last_x = pos_x;
    last_y = pos_y;
    first_mouse = false;
  }

  float x_offset = pos_x - last_x;
  float y_offset = last_y - pos_y;  // reversed since y-coordinates go from bottom to top
  last_x = pos_x;
  last_y = pos_y;
  camera.ProcessMouseMovement(x_offset, y_offset);
}

void scrollCallback(GLFWwindow* window, double x_offset, double y_offset) {
  camera.ProcessMouseScroll(y_offset);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
  // make sure the viewport matches the new window dimensions; note that width and
  // height will be significantly larger than specified on retina displays.
  glViewport(0, 0, width, height);
}

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
  stbi_set_flip_vertically_on_load(true);
  // glfw: initialize and configure
  // ------------------------------
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  // glfw window creation
  // --------------------
  GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Space", NULL, NULL);
  if (window == NULL) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
  glfwSetCursorPosCallback(window, mouseCallback);
  glfwSetScrollCallback(window, scrollCallback);
  // tell GLFW to capture our mouse
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  // glad: load all OpenGL function pointers
  // ---------------------------------------
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  // ObjLoader loader("assets/light_bulb/light_bulb.obj");
  ObjLoader loader("assets/backpack/backpack.obj");
  loader.Load();
  SceneObject scene;
  GLRenderer render;

  // Configure OpenGL state
  glEnable(GL_DEPTH_TEST);

  // Unbind all
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  scene.Add(std::move(loader.object_3d_));
  // render loop
  // -----------
  while (!glfwWindowShouldClose(window)) {
    // Handle time
    float current_frame = glfwGetTime();
    delta_time = current_frame - last_frame;
    last_frame = current_frame;
    // input
    // -----
    processInput(window);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    render.Render(scene, camera);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    // -------------------------------------------------------------------------------
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  // optional: de-allocate all resources once they've outlived their purpose:
  // ------------------------------------------------------------------------
  // glfw: terminate, clearing all previously allocated GLFW resources.
  // ------------------------------------------------------------------
  glfwTerminate();
  return 0;
}
