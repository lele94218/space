#include "gl_renderer.h"

#include <glog/logging.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../errors/gl_error.h"

void GLRenderer::Render(const SceneObject& scene, const CameraObject& camera) {
  if (!initialized_) {
    for (int index = 0; index < scene.size(); ++index) {
      const Object3D* object = scene.get(index);
      render_list_.push_back(object);
    }
    for (const Object3D* object : render_list_) {
      for (int index = 0; index < object->size(); ++index) {
        const Object3D* sub_object = object->get(index);
        if (dynamic_cast<const MeshObject*>(sub_object)) {
          SetupMeshObject(static_cast<const MeshObject*>(sub_object));
        }
      }
    }
    initialized_ = true;
  }

  // TODO: we assume that one scene should use the same shader.
  const GLProgram* program_ptr = GLGlobalResources::GetInstance().programs().at("model_shader").get();
  glUseProgram(program_ptr->program());

  const unsigned int SCR_WIDTH = 800;
  const unsigned int SCR_HEIGHT = 600;

  // View (Camera)
  glm::mat4 view = camera.GetViewMatrix();
  // Projection
  glm::mat4 projection =
      glm::perspective(glm::radians(camera.zoom()), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
  // Model
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
  // model = glm::scale(model, glm::vec3(0.05f));  // a smaller cube

  // lighting
  glm::vec3 light_pos(1.2f, 1.0f, 2.0f);
  program_ptr->SetVector3("light.position", glm::value_ptr(light_pos));
  program_ptr->SetVector3("light.diffuse", glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.8f)));
  program_ptr->SetVector3("light.ambient", glm::value_ptr(glm::vec3(0.1f, 0.1f, 0.1f)));
  program_ptr->SetVector3("light.specular", glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.8f)));
  program_ptr->SetVector3("view_pos", glm::value_ptr(camera.position()));
  program_ptr->SetInt("material.diffuse", 0);
  program_ptr->SetFloat("material.shininess", 32.0f);
  program_ptr->SetMatrix4("projection", glm::value_ptr(projection));
  program_ptr->SetMatrix4("view", glm::value_ptr(view));
  program_ptr->SetMatrix4("model", glm::value_ptr(model));

  for (const Object3D* object : render_list_) {
    for (int index = 0; index < object->size(); ++index) {
      const Object3D* sub_object = object->get(index);
      if (dynamic_cast<const MeshObject*>(sub_object)) {
        RenderMeshObject(static_cast<const MeshObject*>(sub_object), camera);
      }
    }
  }
}

void GLRenderer::SetupMeshObject(const MeshObject* mesh_object) const {
  const Material& material = mesh_object->material();
  const Geometry& geometry = mesh_object->geometry();
  if (!GLGlobalResources::GetInstance().textures().count(material.map_texture_path)) {
    auto ptr = std::make_unique<GLTexture>(material);
    GLGlobalResources::GetInstance().textures().emplace(material.map_texture_path, std::move(ptr));
  }
  if (!GLGlobalResources::GetInstance().binding_states().count(geometry.id)) {
    auto ptr = std::make_unique<GLBindingState>(geometry);
    GLGlobalResources::GetInstance().binding_states().emplace(geometry.id, std::move(ptr));
  }
  if (!GLGlobalResources::GetInstance().programs().count(material.shader_name)) {
    auto ptr = std::make_unique<GLProgram>("shaders/" + material.shader_name + ".vs",
                                           "shaders/" + material.shader_name + ".fs");
    GLGlobalResources::GetInstance().programs().emplace(material.shader_name, std::move(ptr));
  }
}

void GLRenderer::RenderMeshObject(const MeshObject* mesh_object, const CameraObject& camera) const {
  const Material& material = mesh_object->material();
  const Geometry& geometry = mesh_object->geometry();
  const GLTexture* texture =
      GLGlobalResources::GetInstance().textures().at(material.map_texture_path).get();
  const GLBindingState* binding_state =
      GLGlobalResources::GetInstance().binding_states().at(geometry.id).get();
  const GLProgram* program =
      GLGlobalResources::GetInstance().programs().at(material.shader_name).get();
  Draw(*binding_state, *program, *texture, camera, geometry.index_size());
}

void GLRenderer::Draw(const GLBindingState& binding_state,
                      const GLProgram& program,
                      const GLTexture& texture,
                      const CameraObject& camera,
                      unsigned int index_size) const {
  // Bind diffuse texture to unit 0
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture.diffuse_id());
  program.SetInt("texture_diffuse1", 0);

  // Bind specular texture to unit 1 (if available)
  if (texture.has_specular()) {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture.specular_id());
    program.SetInt("texture_specular1", 1);
    program.SetInt("texture_sample", 1);
  } else {
    program.SetInt("texture_sample", 0);
  }

  glBindVertexArray(binding_state.vao());
  glDrawElements(GL_TRIANGLES, index_size, GL_UNSIGNED_INT, 0);

  glBindVertexArray(0);
  glActiveTexture(GL_TEXTURE0);
}
