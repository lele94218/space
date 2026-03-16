# CLAUDE.md — Space 3D Renderer

## Project Overview

A lightweight C++ OpenGL PBR rendering engine inspired by Three.js architecture.
Built with SDL2 (windowing/input) + OpenGL 3.3 Core (rendering) + native glTF loader + Assimp (OBJ fallback).

Originally written in 2023, resumed in 2026.

## Build Commands

```bash
# First time / after CMakeLists changes
cmake . -DCMAKE_POLICY_VERSION_MINIMUM=3.5

# Build
make -j4

# Run
./main

# Headless screenshot
./main --screenshot output.png
```

## Dependencies

```bash
brew install cmake sdl2 glog assimp
```

Note: ImGui is vendored as a git submodule in `deps/imgui/`. nlohmann/json is vendored in `deps/nlohmann/`.

## Project Structure

```
main.cc               # Entry point, SDL2 window, render loop, ImGui debug panel
glad.cc               # OpenGL function loader
stab_image.cc         # STB image implementation
src/
  render_config.h     # Runtime pipeline config (MSAA, shading mode, IBL)
  core/
    object_3d.*       # Base scene graph node (position, rotation, scale)
    mesh_object.*     # Mesh node (geometry + material)
    scene_object.h    # Scene root container
    camera_object.*   # Perspective camera with Euler angles (yaw/pitch)
    geometry.*        # Vertex/index buffer data (position/normal/UV/tangent)
    material.*        # PBR material properties (factors, textures, alpha modes, KHR extensions)
  renders/
    i_renderer.h      # Abstract renderer interface
    gl_renderer.*     # Main renderer — scene graph traversal, two-pass (opaque + transparent)
    gl_program.*      # GLSL shader program wrapper
    gl_texture.*      # OpenGL texture wrapper with caching
    gl_binding_state.*# VAO/VBO/IBO binding state manager
    gl_framebuffer.*  # MSAA FBO + resolve FBO
    gl_ibl.*          # IBL precomputation (HDR → cubemaps, irradiance, BRDF LUT)
    gl_global_resouces.* # Singleton resource cache
  loaders/
    gltf_loader.*     # Native glTF 2.0 / GLB parser (nlohmann/json, no Assimp)
    obj_loader.*      # Assimp-based OBJ loader (legacy fallback)
  errors/
    gl_error.*        # OpenGL error logging
shaders/
  pbr_shader.vs/fs              # Cook-Torrance BRDF + IBL + KHR extensions
  model_shader.vs/fs            # Blinn-Phong fallback (OBJ models)
  material_shader.vs/fs         # Basic material rendering
  light_shader.vs/fs            # Light source debug visualization
  tex_shader.vs/fs              # Texture debug
  t_shader.vs/fs                # Transform debug
  equirect_to_cubemap.vs/fs     # HDR equirectangular → cubemap conversion
  irradiance_convolution.vs/fs  # Diffuse IBL precomputation
  prefilter.vs/fs               # Specular IBL prefiltered cubemap (5 mip levels)
  brdf_lut.vs/fs                # Smith GGX BRDF LUT generation
assets/
  lily/             # Lily GLB model (default)
  backpack/         # Backpack OBJ model with textures
  container/        # Container OBJ test model
  light_bulb/       # Simple OBJ test model
  env/neutral.hdr   # Studio HDR environment map for IBL
deps/                 # Vendored: glad, glm, imgui (submodule), nlohmann/json, KHR, stb_image
deprecated/           # Old code (not used)
test/                 # Google Test unit tests
```

## Key Classes

| Class | Role |
|-------|------|
| `Object3D` | Base node with transform (position/rotation/scale) |
| `MeshObject` | Renderable node with Geometry + Material |
| `CameraObject` | Perspective camera with yaw/pitch, inherits Object3D |
| `IRenderer` | Abstract renderer interface |
| `GLRenderer` | Traverses scene graph, two-pass render (opaque + transparent), PBR + Blinn-Phong |
| `GLProgram` | Wraps GLSL compile/link/uniform binding |
| `GLTexture` | Wraps GL texture creation/binding with caching |
| `GLBindingState` | Manages VAO/VBO/IBO per geometry |
| `GLFramebuffer` | MSAA FBO with configurable sample count + resolve pass |
| `GLIBL` | IBL precomputation (irradiance, prefilter, BRDF LUT) |
| `GLTFLoader` | Native glTF 2.0 / GLB parser |
| `ObjLoader` | Assimp-based OBJ loader |
| `RenderConfig` | Runtime pipeline config struct |

## Controls

- **Right-drag**: Orbit camera (Blender-style)
- **WASD**: Move camera
- **Scroll wheel**: Zoom
- **F key**: Toggle fullscreen
- **Tab key**: Toggle mouse capture (free mouse for ImGui)

## Development Notes

- C++17 standard
- GLM for math (vec3, mat4)
- SDL2 handles window creation and mouse/keyboard input
- VSync enabled by default via `SDL_GL_SetSwapInterval(1)`
- MSAA 8x by default (configurable via ImGui panel)
- ImGui debug panel: model selector, PBR sliders, light/IBL controls, camera reset
- `--screenshot [path]` mode: renders 3 seconds then saves PNG via glReadPixels

## Implemented Features

- PBR rendering (Cook-Torrance BRDF: GGX NDF, Smith Geometry, Schlick Fresnel)
- IBL (diffuse irradiance + specular split-sum approximation)
- ACESFilmic tone mapping
- Normal mapping (TBN with Gram-Schmidt re-orthogonalization)
- MSAA anti-aliasing (1/2/4/8x via offscreen FBO)
- Alpha blending (two-pass: opaque then transparent)
- Native glTF 2.0 / GLB loading (spec-compliant, no Assimp)
- KHR_materials_clearcoat + KHR_materials_specular extensions
- Blinn-Phong fallback for OBJ models
- ImGui debug panel with runtime controls
- Headless screenshot mode
- IRenderer abstract interface

## Roadmap

### Done
- [x] VSync / frame rate control
- [x] ImGui debug panel
- [x] Blinn-Phong shading
- [x] Normal mapping
- [x] PBR (Cook-Torrance BRDF)
- [x] IBL (diffuse + specular split-sum)
- [x] HDR + Tone Mapping (ACESFilmic)
- [x] MSAA anti-aliasing
- [x] Native glTF 2.0 loader
- [x] Texture/resource caching

### Next Up
- [ ] Shadow Mapping
- [ ] Skybox rendering
- [ ] Multi-light support (Light scene nodes)
- [ ] Post-processing framework (FBO chain)
- [ ] Skeletal animation
- [ ] Subsurface scattering (SSS)
- [ ] Morph targets

### Future
- [ ] ECS architecture refactor
- [ ] Cross-platform CMake (Linux/Windows)
- [ ] Physics integration (Bullet or Jolt)
- [ ] Audio (SDL_mixer or OpenAL)
- [ ] Scripting (Lua)
