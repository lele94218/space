#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glog/logging.h>
#include <stab/stab_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include "camera.h"
#include "error.h"
#include "model.h"
#include "shader.h"

float delta_time = 0.0f;  // Time between current frame and last frame
float last_frame = 0.0f;  // Time of last frame
bool first_mouse = true;
// lighting
glm::vec3 light_pos(1.2f, 1.0f, 2.0f);

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
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
  GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
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

  // load shader
  Shader model_shader("shaders/model_shader.vs", "shaders/model_shader.fs");
  Shader light_shader("shaders/light_shader.vs", "shaders/light_shader.fs");

  // load model
  Model backpack_model("assets/backpack/backpack.obj");
  // Model backpack_model("assets/light_bulb/light_bulb.obj");
  Model light_model("assets/light_bulb/light_bulb.obj");

  // Configure OpenGL state
  glEnable(GL_DEPTH_TEST);

  // Unbind all
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  glCheckError();
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

    model_shader.use();
    // Model shader properties
    model_shader.setVector3("light.position", glm::value_ptr(light_pos));
    model_shader.setVector3("light.diffuse", glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.8f)));
    model_shader.setVector3("light.ambient", glm::value_ptr(glm::vec3(0.1f, 0.1f, 0.1f)));
    model_shader.setVector3("light.specular", glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.8f)));
    model_shader.setVector3("view_pos", glm::value_ptr(camera.position()));
    model_shader.setInt("material.diffuse", 0);
    model_shader.setFloat("material.shininess", 32.0f);
    // View (Camera)
    glm::mat4 view = camera.GetViewMatrix();
    // Projection
    glm::mat4 projection;
    projection =
        glm::perspective(glm::radians(camera.zoom()), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
    model_shader.setMatrix4("projection", glm::value_ptr(projection));
    model_shader.setMatrix4("view", glm::value_ptr(view));

    // render the loaded model
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    // translate it down so it's at the center of the scene
    model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
    // it's a bit too big for our scene, so scale it down
    model_shader.setMatrix4("model", glm::value_ptr(model));
    backpack_model.Draw(model_shader);

    // Draw the light
    light_shader.use();
    light_shader.setMatrix4("projection", glm::value_ptr(projection));
    light_shader.setMatrix4("view", glm::value_ptr(view));
    model = glm::mat4(1.0f);
    model = glm::translate(model, light_pos);
    model = glm::scale(model, glm::vec3(0.05f));  // a smaller cube
    light_shader.setMatrix4("model", glm::value_ptr(model));
    // Render the light
    light_model.Draw(light_shader);


    glCheckError();
    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    // -------------------------------------------------------------------------------
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  // optional: de-allocate all resources once they've outlived their purpose:
  // ------------------------------------------------------------------------
  model_shader.reset();
  light_shader.reset();
  // glfw: terminate, clearing all previously allocated GLFW resources.
  // ------------------------------------------------------------------
  glfwTerminate();
  return 0;
}
