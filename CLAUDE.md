# CLAUDE.md — Space 3D Renderer

## Project Overview

A lightweight C++ OpenGL 3D rendering engine inspired by Three.js architecture.
Built with SDL2 (windowing/input) + OpenGL (rendering) + Assimp (model loading).

Originally written in 2023, resumed in 2026.

## Build Commands

```bash
# First time / after CMakeLists changes
cmake . -DCMAKE_POLICY_VERSION_MINIMUM=3.5

# Build
make -j4

# Run
./main
```

## Dependencies

```bash
brew install cmake glfw glog assimp
```

## Project Structure

```
main.cc               # Entry point, SDL2 window, render loop
glad.cc               # OpenGL function loader
src/
  core/
    object_3d.*       # Base scene graph node (position, rotation, scale)
    mesh_object.*     # Mesh node (geometry + material)
    scene_object.*    # Scene root
    camera_object.*   # Camera (view + projection matrices)
    geometry.*        # Vertex/index buffer data
    material.*        # Material properties (color, texture refs)
  renders/
    gl_renderer.*     # Main renderer — draws scene graph
    gl_program.*      # GLSL shader program wrapper
    gl_texture.*      # OpenGL texture wrapper
    gl_binding_state.*# VAO/VBO binding state manager
    gl_global_resouces.* # Shared GL resources
  loaders/            # Assimp-based model loaders
  errors/             # Error types
shaders/              # GLSL vertex + fragment shaders
  model_shader.vs/fs  # For loaded 3D models
  material_shader.vs/fs # For material-based objects
  light_shader.vs/fs  # Lighting
  tex_shader.vs/fs    # Texture mapping
  t_shader.vs/fs      # Basic transform
deps/                 # Vendored headers (glad, stb_image, etc.)
test/                 # Google Test unit tests
```

## Key Classes

| Class | Role |
|-------|------|
| `Object3D` | Base node with transform (position/rotation/scale) |
| `MeshObject` | Renderable node with Geometry + Material |
| `CameraObject` | View/projection, inherits Object3D |
| `GLRenderer` | Traverses scene graph, issues GL draw calls |
| `GLProgram` | Wraps GLSL compile/link/uniform binding |
| `GLTexture` | Wraps GL texture creation/binding |
| `GLBindingState` | Manages VAO per geometry |

## Development Notes

- C++14 standard
- No external math library — uses raw arrays/structs (consider adding GLM)
- SDL2 handles window creation and mouse/keyboard input
- Camera orbit implemented via mouse drag in `main.cc`
- VSync is NOT enabled (FPS uncapped, ~2000+ in windowed mode)

## Next Steps / Ideas

- [ ] Add GLM for cleaner math (vec3, mat4)
- [ ] PBR materials (roughness, metallic, normal maps)
- [ ] Shadow mapping
- [ ] Skybox / IBL
- [ ] ImGui debug overlay
- [ ] Frame time cap / VSync
