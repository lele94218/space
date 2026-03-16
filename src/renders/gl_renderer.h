#pragma once

#include "../core/camera_object.h"
#include "../core/mesh_object.h"
#include "../core/scene_object.h"
#include "gl_global_resouces.h"
#include "i_renderer.h"

struct RenderConfig;  // forward declaration

class GLRenderer : public IRenderer {
 public:
  GLRenderer() = default;

  void Render(const SceneObject& scene, const CameraObject& camera) override;
  void SetConfig(RenderConfig* config) { config_ = config; }
  void Reset() { render_list_.clear(); initialized_ = false; }

 private:
  void SetupMeshObject(const MeshObject* mesh_object) const;
  void RenderMeshObject(const MeshObject* object, const CameraObject& camera) const;
  void DrawBlinnPhong(const GLBindingState& binding_state,
                      const GLProgram& program,
                      const GLTexture& texture,
                      const Material& material,
                      unsigned int index_size) const;
  void DrawPBR(const GLBindingState& binding_state,
               const GLProgram& program,
               const GLTexture& texture,
               const Material& material,
               unsigned int index_size) const;

  std::vector<const Object3D*> render_list_;
  bool initialized_ = false;
  RenderConfig* config_ = nullptr;
};
