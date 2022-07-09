#pragma once

#include "../core/camera_object.h"
#include "../core/mesh_object.h"
#include "../core/scene_object.h"
#include "gl_global_resouces.h"

class GLRenderer {
 public:
  GLRenderer() = default;

  void Render(const SceneObject& scene, const CameraObject& camera);

 private:
  void SetupMeshObject(const MeshObject* mesh_object) const;
  void RenderMeshObject(const MeshObject* object, const CameraObject& camera) const;
  void Draw(const GLBindingState& binding_state,
            const GLProgram& program,
            const GLTexture& texture,
            const CameraObject& camera,
            unsigned int index_size) const;

  std::vector<const Object3D*> render_list_;
  // TODO: should support multiple render lists later.
  bool initialized_ = false;
};
