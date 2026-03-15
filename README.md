# Space — C++ OpenGL 3D Renderer

A lightweight 3D rendering engine built with OpenGL, inspired by the architecture of [Three.js](https://threejs.org/).

## Features

- **Scene graph** — `Object3D`, `MeshObject`, `SceneObject` hierarchy
- **Geometry & Materials** — decoupled mesh data from shading
- **Camera** — `CameraObject` with view/projection support
- **Renderer** — `GLRenderer` with OpenGL backend (VAO/VBO, shader programs, textures, binding state)
- **Model loading** — via [Assimp](https://github.com/assimp/assimp) (`.obj`, `.fbx`, etc.)
- **Mouse input** — camera orbit/pan via SDL2
- **GLSL shaders** — multiple shader programs (texture, material, lighting, model)

## Architecture

```
src/
├── core/         # Object3D, MeshObject, SceneObject, Camera, Geometry, Material
├── renders/      # GLRenderer, GLProgram, GLTexture, GLBindingState, GLGlobalResources
├── loaders/      # Asset loaders (Assimp wrapper)
└── errors/       # Error handling
shaders/          # GLSL vertex & fragment shaders
```

## Requirements

- macOS (Apple Silicon / Intel)
- CMake ≥ 3.5
- C++14

## Installation

```bash
brew install cmake glfw glog assimp
```

## Build & Run

```bash
git clone https://github.com/lele94218/space.git
cd space
cmake . -DCMAKE_POLICY_VERSION_MINIMUM=3.5
make -j4
./main
```

## Controls

- **Mouse drag** — orbit camera
- **Q / ESC** — quit

## Status

Active development resumed in 2026.
