#include <SDL.h>
#include <glad/glad.h>
#include <glog/logging.h>
#include <stab/stab_image.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <memory>

#include "src/core/camera_object.h"
#include "src/core/scene_object.h"
#include "src/errors/gl_error.h"
#include "src/loaders/obj_loader.h"
#include "src/render_config.h"
#include "src/renders/gl_framebuffer.h"
#include "src/renders/gl_renderer.h"
#include "src/renders/i_renderer.h"

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  stbi_set_flip_vertically_on_load(true);

  SDL_Window* window = nullptr;
  const int SCR_WIDTH  = 900;
  const int SCR_HEIGHT = 600;
  float last_time  = 0.0f;
  float total_time = 0.0f;
  int   frame_count = 0;

  if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
    LOG(ERROR) << "SDL could not be initialized: " << SDL_GetError();
  } else {
    LOG(ERROR) << "SDL video system is ready to go";
  }

  bool rightMouseDown = false;
  int  lastMouseX = 0, lastMouseY = 0;

  // OpenGL 3.3 Core
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  // No SDL-level MSAA — we manage our own MSAA FBO
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);

  window = SDL_CreateWindow("Space",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    SCR_WIDTH, SCR_HEIGHT,
    SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);

  SDL_GLContext context = SDL_GL_CreateContext(window);
  gladLoadGLLoader(SDL_GL_GetProcAddress);
  SDL_GL_SetSwapInterval(1); // VSync

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_MULTISAMPLE); // needed for MSAA FBO to work

  // ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui::StyleColorsDark();
  ImGui_ImplSDL2_InitForOpenGL(window, context);
  ImGui_ImplOpenGL3_Init("#version 330 core");

  // Scene
  CameraObject camera;
  ObjLoader loader("assets/backpack/backpack.obj");
  loader.Load();
  SceneObject scene;
  auto gl_renderer = std::make_unique<GLRenderer>();
  RenderConfig config;
  gl_renderer->SetConfig(&config);
  std::unique_ptr<IRenderer> renderer = std::move(gl_renderer);
  scene.Add(std::move(loader.object_3d_));

  // MSAA FBO
  std::unique_ptr<GLFramebuffer> fbo =
      std::make_unique<GLFramebuffer>(SCR_WIDTH, SCR_HEIGHT, config.msaa_samples);

  // For rebuilding FBO when config changes
  bool config_dirty = false;
  int  pending_samples = config.msaa_samples;

  bool gameIsRunning = true;
  while (gameIsRunning) {
    SDL_Event event;
    float current_time = SDL_GetTicks() / 1000.0f;
    float delta_time   = current_time - last_time;
    last_time = current_time;

    // FPS log
    frame_count++;
    total_time += delta_time;
    if (frame_count == 100) {
      LOG(ERROR) << std::fixed << "FPS: " << (frame_count / total_time);
      frame_count = 0;
      total_time  = 0;
    }

    // Keyboard movement
    const Uint8* state = SDL_GetKeyboardState(NULL);
    if (state[SDL_SCANCODE_W]) camera.ProcessKeyboard(CameraMovement::FORWARD,  delta_time);
    if (state[SDL_SCANCODE_S]) camera.ProcessKeyboard(CameraMovement::BACKWARD, delta_time);
    if (state[SDL_SCANCODE_A]) camera.ProcessKeyboard(CameraMovement::LEFT,     delta_time);
    if (state[SDL_SCANCODE_D]) camera.ProcessKeyboard(CameraMovement::RIGHT,    delta_time);

    // Right-drag: rotate view
    if (rightMouseDown) {
      int mouseX, mouseY;
      SDL_GetMouseState(&mouseX, &mouseY);
      int dx = mouseX - lastMouseX;
      int dy = mouseY - lastMouseY;
      lastMouseX = mouseX;
      lastMouseY = mouseY;
      if (dx != 0 || dy != 0)
        camera.ProcessMouseMovement(dx, -dy);
    }

    // Event loop
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT) gameIsRunning = false;
      if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
        if (!io.WantCaptureMouse) {
          rightMouseDown = true;
          SDL_GetMouseState(&lastMouseX, &lastMouseY);
        }
      }
      if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_RIGHT)
        rightMouseDown = false;
    }

    // Rebuild FBO if config changed
    if (config_dirty) {
      config.msaa_samples = pending_samples;
      fbo->Rebuild(SCR_WIDTH, SCR_HEIGHT, config.msaa_samples);
      config_dirty = false;
    }

    // --- Render scene into MSAA FBO ---
    if (config.msaa_enabled) {
      fbo->Bind();
    } else {
      GLFramebuffer::BindDefault();
      glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    }

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderer->Render(scene, camera);

    // --- Resolve MSAA → default framebuffer ---
    if (config.msaa_enabled) {
      fbo->Resolve();
      // Blit resolved image to screen
      glBindFramebuffer(GL_READ_FRAMEBUFFER, 0); // resolve_fbo already unbound; blit from default after resolve
      // Actually blit resolve FBO → default (screen)
      glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo->resolve_fbo_pub());
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
      glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT,
                        0, 0, SCR_WIDTH, SCR_HEIGHT,
                        GL_COLOR_BUFFER_BIT, GL_NEAREST);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    }

    // --- ImGui (always on default framebuffer) ---
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Debug");
    ImGui::Text("FPS: %.1f", io.Framerate);
    ImGui::Text("Camera: (%.1f, %.1f, %.1f)",
      camera.position().x, camera.position().y, camera.position().z);
    if (ImGui::Button("Reset Camera")) camera.Reset();

    ImGui::Separator();
    ImGui::Text("Rendering");
    ImGui::Checkbox("MSAA", &config.msaa_enabled);
    ImGui::Checkbox("PBR", &config.use_pbr);
    if (config.use_pbr) {
      ImGui::SliderFloat("Metallic",  &config.pbr_metallic,  0.0f, 1.0f);
      ImGui::SliderFloat("Roughness", &config.pbr_roughness, 0.0f, 1.0f);
    }
    // Sample count selector (only meaningful when MSAA is on)
    if (config.msaa_enabled) {
      const char* sample_items[] = { "1x", "2x", "4x", "8x" };
      const int   sample_vals[]  = { 1, 2, 4, 8 };
      int current = 0;
      for (int i = 0; i < 4; i++) if (sample_vals[i] == config.msaa_samples) current = i;
      if (ImGui::Combo("Samples", &current, sample_items, 4)) {
        pending_samples = sample_vals[current];
        config_dirty = true;
      }
    }

    ImGui::Separator();
    ImGui::Text("Right-drag: rotate view");
    ImGui::Text("WASD: move");
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
