#include "gl_renderer.h"

#include <glog/logging.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../errors/gl_error.h"
#include "../render_config.h"

static const int SCR_WIDTH  = 900;
static const int SCR_HEIGHT = 600;

void GLRenderer::Render(const SceneObject& scene, const CameraObject& camera) {
  // Determine shader to use based on config
  const std::string shader_name = (config_ && config_->use_pbr) ? "pbr_shader" : "model_shader";

  if (!initialized_) {
    for (int index = 0; index < scene.size(); ++index) {
      render_list_.push_back(scene.get(index));
    }
    // Pre-load pbr_shader as well so it's ready to switch
    if (!GLGlobalResources::GetInstance().programs().count("pbr_shader")) {
      auto ptr = std::make_unique<GLProgram>("shaders/pbr_shader.vs", "shaders/pbr_shader.fs");
      GLGlobalResources::GetInstance().programs().emplace("pbr_shader", std::move(ptr));
    }
    for (const Object3D* object : render_list_) {
      for (int index = 0; index < object->size(); ++index) {
        const Object3D* sub = object->get(index);
        if (dynamic_cast<const MeshObject*>(sub))
          SetupMeshObject(static_cast<const MeshObject*>(sub));
      }
    }
    initialized_ = true;
  }

  const GLProgram* program_ptr =
      GLGlobalResources::GetInstance().programs().at(shader_name).get();
  glUseProgram(program_ptr->program());

  glm::mat4 view       = camera.GetViewMatrix();
  glm::mat4 projection = glm::perspective(
      glm::radians(camera.zoom()),
      (float)SCR_WIDTH / (float)SCR_HEIGHT,
      0.1f, 100.0f);
  glm::mat4 model = glm::mat4(1.0f);

  glm::vec3 light_pos(5.0f, 5.0f, 5.0f);

  if (config_ && config_->use_pbr) {
    // PBR uniforms
    program_ptr->SetVector3("light_pos",   glm::value_ptr(light_pos));
    program_ptr->SetVector3("light_color", glm::value_ptr(glm::vec3(150.0f)));
    program_ptr->SetVector3("view_pos",    glm::value_ptr(camera.position()));
    program_ptr->SetFloat("u_metallic",    config_->pbr_metallic);
    program_ptr->SetFloat("u_roughness",   config_->pbr_roughness);
    program_ptr->SetFloat("u_ao",          1.0f);
  } else {
    // Blinn-Phong uniforms
    program_ptr->SetVector3("light.position", glm::value_ptr(light_pos));
    program_ptr->SetVector3("light.diffuse",  glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.8f)));
    program_ptr->SetVector3("light.ambient",  glm::value_ptr(glm::vec3(0.1f, 0.1f, 0.1f)));
    program_ptr->SetVector3("light.specular", glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.8f)));
    program_ptr->SetVector3("view_pos",       glm::value_ptr(camera.position()));
    program_ptr->SetFloat("material.shininess", 32.0f);
  }

  program_ptr->SetMatrix4("projection", glm::value_ptr(projection));
  program_ptr->SetMatrix4("view",       glm::value_ptr(view));
  program_ptr->SetMatrix4("model",      glm::value_ptr(model));

  for (const Object3D* object : render_list_) {
    for (int index = 0; index < object->size(); ++index) {
      const Object3D* sub = object->get(index);
      if (dynamic_cast<const MeshObject*>(sub))
        RenderMeshObject(static_cast<const MeshObject*>(sub), camera);
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
    auto ptr = std::make_unique<GLProgram>(
        "shaders/" + material.shader_name + ".vs",
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

  const std::string shader_name = (config_ && config_->use_pbr) ? "pbr_shader" : "model_shader";
  const GLProgram* program =
      GLGlobalResources::GetInstance().programs().at(shader_name).get();

  if (config_ && config_->use_pbr)
    DrawPBR(*binding_state, *program, *texture, geometry.index_size());
  else
    DrawBlinnPhong(*binding_state, *program, *texture, geometry.index_size());
}

void GLRenderer::DrawBlinnPhong(const GLBindingState& binding_state,
                                const GLProgram& program,
                                const GLTexture& texture,
                                unsigned int index_size) const {
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture.diffuse_id());
  program.SetInt("texture_diffuse1", 0);

  if (texture.has_specular()) {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture.specular_id());
    program.SetInt("texture_specular1", 1);
    program.SetInt("texture_sample", 1);
  } else {
    program.SetInt("texture_sample", 0);
  }

  if (texture.has_normal()) {
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texture.normal_id());
    program.SetInt("texture_normal1", 2);
    program.SetInt("use_normal_map", 1);
  } else {
    program.SetInt("use_normal_map", 0);
  }

  glBindVertexArray(binding_state.vao());
  glDrawElements(GL_TRIANGLES, index_size, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
  glActiveTexture(GL_TEXTURE0);
}

void GLRenderer::DrawPBR(const GLBindingState& binding_state,
                         const GLProgram& program,
                         const GLTexture& texture,
                         unsigned int index_size) const {
  // unit 0: albedo
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture.diffuse_id());
  program.SetInt("texture_albedo", 0);

  // unit 1: metallic (specular map)
  if (texture.has_specular()) {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture.specular_id());
    program.SetInt("texture_specular", 1);
    program.SetInt("use_metallic", 1);
  } else {
    program.SetInt("use_metallic", 0);
  }

  // unit 2: normal map
  if (texture.has_normal()) {
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texture.normal_id());
    program.SetInt("texture_normal", 2);
    program.SetInt("use_normal_map", 1);
  } else {
    program.SetInt("use_normal_map", 0);
  }

  // unit 3: roughness
  if (texture.has_roughness()) {
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, texture.roughness_id());
    program.SetInt("texture_roughness", 3);
    program.SetInt("use_roughness", 1);
  } else {
    program.SetInt("use_roughness", 0);
  }

  // unit 4: ambient occlusion
  if (texture.has_ao()) {
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, texture.ao_id());
    program.SetInt("texture_ao", 4);
    program.SetInt("use_ao", 1);
  } else {
    program.SetInt("use_ao", 0);
  }

  glBindVertexArray(binding_state.vao());
  glDrawElements(GL_TRIANGLES, index_size, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
  glActiveTexture(GL_TEXTURE0);
}
