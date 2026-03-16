#include <SDL.h>
#include <glad/glad.h>
#include <glog/logging.h>
#include <stab/stab_image.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

#include <iostream>

#include "src/core/camera_object.h"
#include "src/core/scene_object.h"
#include "src/errors/gl_error.h"
#include "src/loaders/obj_loader.h"
#include "src/renders/gl_renderer.h"

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
  stbi_set_flip_vertically_on_load(true);
  // Create a window data type This pointer will point to the window that is allocated from
  // SDL_CreateWindow
  SDL_Window* window = nullptr;

  const unsigned int SCR_WIDTH = 900;
  const unsigned int SCR_HEIGHT = 600;
  float last_time = 0.0f;
  float total_time = 0.0f;
  int frame_count = 0;

  // Initialize the video subsystem. If it returns less than 1, then an error code will be received.
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
    LOG(ERROR) << "SDL could not be initialized: " << SDL_GetError();
  } else {
    LOG(ERROR) << "SDL video system is ready to go\n";
  }
  // Start with normal cursor (not captured) so ImGui works
  // Press Tab to toggle mouse capture for camera look
  bool mouseCaptured = false;
  // Before we create our window, specify OpenGL version
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  // Settings
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 32);

  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);

  // SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

  // Request a window to be created for our platform The parameters are for the title, x and y
  // position, and the width and height of the window.
  window = SDL_CreateWindow("Space", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCR_WIDTH, SCR_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
  // SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

  // OpenGL setup the graphics context
  SDL_GLContext context;
  context = SDL_GL_CreateContext(window);

  // Setup our function pointers
  gladLoadGLLoader(SDL_GL_GetProcAddress);

  // VSync
  SDL_GL_SetSwapInterval(1);

  // Configure OpenGL state
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_MULTISAMPLE);

  // ImGui setup
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui::StyleColorsDark();
  ImGui_ImplSDL2_InitForOpenGL(window, context);
  ImGui_ImplOpenGL3_Init("#version 330 core");

  // camera
  CameraObject camera;
  // ObjLoader loader("assets/light_bulb/light_bulb.obj");
  ObjLoader loader("assets/backpack/backpack.obj");
  loader.Load();
  SceneObject scene;
  GLRenderer render;
  scene.Add(std::move(loader.object_3d_));

  // Infinite loop for our application
  bool gameIsRunning = true;
  while (gameIsRunning) {
    SDL_Event event;
    float current_time = SDL_GetTicks() / 1000.0;
    float delta_time = current_time - last_time;
    last_time = current_time;

    // FPS
    frame_count++;
    total_time += delta_time;
    if (frame_count == 100) {
      float fps = frame_count / total_time;
      frame_count = 0;
      total_time = 0;
      LOG(ERROR) << std::fixed << "FPS: " << fps;
    }

    // Retrieve the state of all of the keys Then we can query the scan code of one or more keys
    // at a time
    const Uint8* state = SDL_GetKeyboardState(NULL);
    if (state[SDL_SCANCODE_W]) {
      camera.ProcessKeyboard(CameraMovement::FORWARD, delta_time);
    }
    if (state[SDL_SCANCODE_S]) {
      camera.ProcessKeyboard(CameraMovement::BACKWARD, delta_time);
    }
    if (state[SDL_SCANCODE_A]) {
      camera.ProcessKeyboard(CameraMovement::LEFT, delta_time);
    }
    if (state[SDL_SCANCODE_D]) {
      camera.ProcessKeyboard(CameraMovement::RIGHT, delta_time);
    }

    // Mouse look only when captured (Tab to toggle)
    if (mouseCaptured) {
      int x_offset, y_offset;
      SDL_GetRelativeMouseState(&x_offset, &y_offset);
      camera.ProcessMouseMovement(x_offset, -y_offset);
    }

    // Start our event loop
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT) {
        gameIsRunning = false;
      }
      // Tab: toggle mouse capture for camera look
      if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_TAB) {
        mouseCaptured = !mouseCaptured;
        SDL_SetRelativeMouseMode(mouseCaptured ? SDL_TRUE : SDL_FALSE);
      }
    }

    // ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Debug panel
    ImGui::Begin("Debug");
    ImGui::Text("FPS: %.1f", io.Framerate);
    ImGui::Text("Camera pos: (%.2f, %.2f, %.2f)",
      camera.position().x, camera.position().y, camera.position().z);
    if (ImGui::Button("Reset Camera")) {
      camera.Reset();
    }
    ImGui::Separator();
    ImGui::Text("Tab: toggle mouse look (%s)", mouseCaptured ? "ON" : "OFF");
    ImGui::Text("WASD: move  Mouse: look");
    ImGui::End();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    render.Render(scene, camera);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(window);
  }

  // We destroy our window. We are passing in the pointer that points to the memory allocated by the
  // 'SDL_CreateWindow' function. Remember, this is a 'C-style' API, we don't have destructors.
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_DestroyWindow(window);

  SDL_Quit();
  return 0;
}
