#include "gl_renderer.h"

#include <glog/logging.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../errors/gl_error.h"

void GLRenderer::Render(const SceneObject& scene, const CameraObject& camera) {
  for (int index = 0; index < scene.size(); ++index) {
    const Object3D* object = scene.get(index);
    render_list_.push_back(object);
  }
  for (const Object3D* object : render_list_) {
    for (int index = 0; index < object->size(); ++index) {
      const Object3D* sub_object = object->get(index);
      if (dynamic_cast<const MeshObject*>(sub_object)) {
        RenderMeshObject(static_cast<const MeshObject*>(sub_object), camera);
      } else {
      }
    }
  }
}

void GLRenderer::RenderMeshObject(const MeshObject* mesh_object, const CameraObject& camera) const {
  const Material& material = mesh_object->material();
  const Geometry& geometry = mesh_object->geometry();
  const GLTexture* texture;

  // TODO: move belowing parts to global resources.
  if (!GLGlobalResources::GetInstance().textures().count(material.id)) {
    auto ptr = std::make_unique<GLTexture>(material);
    GLGlobalResources::GetInstance().textures().emplace(material.id, std::move(ptr));
  }
  texture = GLGlobalResources::GetInstance().textures().at(material.id).get();

  const GLBindingState* binding_state;
  if (!GLGlobalResources::GetInstance().binding_states().count(geometry.id)) {
    auto ptr = std::make_unique<GLBindingState>(geometry);
    GLGlobalResources::GetInstance().binding_states().emplace(geometry.id, std::move(ptr));
    LOG(ERROR) << geometry.id;
  }
  binding_state = GLGlobalResources::GetInstance().binding_states().at(geometry.id).get();

  const GLProgram* program;
  if (!GLGlobalResources::GetInstance().programs().count(material.id)) {
    auto ptr = std::make_unique<GLProgram>("shaders/light_shader.vs", "shaders/light_shader.fs");
    GLGlobalResources::GetInstance().programs().emplace(geometry.id, std::move(ptr));
  }
  program = GLGlobalResources::GetInstance().programs().at(material.id).get();

  Draw(*binding_state, *program, *texture, geometry.index_size());
}

void GLRenderer::Draw(const GLBindingState& binding_state,
                      const GLProgram& program,
                      const GLTexture& texture,
                      unsigned int index_size) const {
  glUseProgram(program.program());
  //  // active proper texture unit before binding
  //  glActiveTexture(GL_TEXTURE0);
  //  program.SetInt("texture_diffuse1", 0);
  //  // and finally bind the texture
  //  glBindTexture(GL_TEXTURE_2D, texture.id());

  //
  const unsigned int SCR_WIDTH = 800;
  const unsigned int SCR_HEIGHT = 600;
  glm::mat4 model = glm::mat4(1.0f);
  glm::mat4 projection =
      glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
  glm::vec3 position(0.0f, 0.0f, 20.0f);
  glm::mat4 view =
      glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  glm::vec3 light_pos(1.2f, 1.0f, 2.0f);
  model = glm::translate(model, light_pos);
  model = glm::scale(model, glm::vec3(0.05f));  // a smaller cube
  program.SetMatrix4("projection", glm::value_ptr(projection));
  program.SetMatrix4("view", glm::value_ptr(view));
  program.SetMatrix4("model", glm::value_ptr(model));
  //

  GLCheckError();
  glBindVertexArray(binding_state.vao());
  GLCheckError();
  glDrawElements(GL_TRIANGLES, index_size, GL_UNSIGNED_INT, 0);
  GLCheckError();
}
