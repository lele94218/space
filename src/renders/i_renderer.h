#pragma once

#include "../core/camera_object.h"
#include "../core/scene_object.h"

// Abstract renderer interface.
// Concrete implementations: GLRenderer, (future) VkRenderer, MetalRenderer.
class IRenderer {
 public:
  virtual ~IRenderer() = default;
  virtual void Render(const SceneObject& scene, const CameraObject& camera) = 0;
};
