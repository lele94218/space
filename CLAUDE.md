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
- GLM already used for math (vec3, mat4)
- SDL2 handles window creation and mouse/keyboard input
- Camera orbit implemented via mouse drag in `main.cc`
- VSync is NOT enabled (FPS uncapped, ~2000+ in windowed mode)

## Roadmap

### 🔴 P0 — 基础设施
- [ ] VSync / 帧率控制 — `SDL_GL_SetSwapInterval(1)` 防止烤 CPU
- [ ] ImGui 集成 — 调试面板，后续所有工作都依赖它

### 🟠 P1 — 渲染核心
- [ ] Phong / Blinn-Phong 光照 — 点光源、方向光、环境光
- [ ] 多光源支持 — Renderer 支持 Light 节点
- [ ] 法线贴图 (Normal Map)
- [ ] PBR 材质 — roughness + metallic + albedo

### 🟡 P2 — 质量提升
- [ ] Shadow Mapping — 方向光阴影
- [ ] Skybox / IBL — 环境光照
- [ ] 后处理框架 — Framebuffer → 屏幕四边形 → shader 链
- [ ] HDR + Tone Mapping

### 🟢 P3 — 工程质量
- [ ] ECS 架构重构
- [ ] 资源管理 — 纹理/Mesh 缓存
- [ ] 跨平台 CMake — Linux/Windows 构建验证

### 🔵 P4 — 游戏化
- [ ] 物理集成 — Bullet 或 Jolt
- [ ] 音频 — SDL_mixer 或 OpenAL
- [ ] Scripting — Lua
