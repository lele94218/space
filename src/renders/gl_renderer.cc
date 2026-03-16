#include "gl_renderer.h"

#include <glog/logging.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../core/material.h"
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
  float intensity = 30.0f;
  if (config_) {
    light_pos = glm::vec3(config_->light_pos[0], config_->light_pos[1], config_->light_pos[2]);
    intensity = config_->light_intensity;
  }

  if (config_ && config_->use_pbr) {
    // PBR uniforms
    program_ptr->SetVector3("light_pos",   glm::value_ptr(light_pos));
    program_ptr->SetVector3("light_color", glm::value_ptr(glm::vec3(intensity)));
    program_ptr->SetVector3("view_pos",    glm::value_ptr(camera.position()));
    program_ptr->SetFloat("u_metallic",    config_->pbr_metallic);
    program_ptr->SetFloat("u_roughness",   config_->pbr_roughness);
    program_ptr->SetFloat("u_ao",          1.0f);
  } else {
    // Blinn-Phong uniforms — normalize intensity to 0-2 range
    float d = intensity / 50.0f;
    program_ptr->SetVector3("light.position", glm::value_ptr(light_pos));
    program_ptr->SetVector3("light.diffuse",  glm::value_ptr(glm::vec3(d, d, d * 0.8f)));
    program_ptr->SetVector3("light.ambient",  glm::value_ptr(glm::vec3(0.1f)));
    program_ptr->SetVector3("light.specular", glm::value_ptr(glm::vec3(d, d, d * 0.8f)));
    program_ptr->SetVector3("view_pos",       glm::value_ptr(camera.position()));
    program_ptr->SetFloat("material.shininess", 32.0f);
  }

  program_ptr->SetMatrix4("projection", glm::value_ptr(projection));
  program_ptr->SetMatrix4("view",       glm::value_ptr(view));
  program_ptr->SetMatrix4("model",      glm::value_ptr(model));

  // Two-pass: opaque first, then alpha-blended (correct transparency layering)
  glDisable(GL_BLEND);
  for (const Object3D* object : render_list_) {
    for (int index = 0; index < object->size(); ++index) {
      const Object3D* sub = object->get(index);
      if (auto* mesh = dynamic_cast<const MeshObject*>(sub)) {
        if (mesh->material().alpha_mode == 0)
          RenderMeshObject(mesh, camera);
      }
    }
  }

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_FALSE);  // BLEND pass: keep depth test but don't write depth
  for (const Object3D* object : render_list_) {
    for (int index = 0; index < object->size(); ++index) {
      const Object3D* sub = object->get(index);
      if (auto* mesh = dynamic_cast<const MeshObject*>(sub)) {
        if (mesh->material().alpha_mode != 0)
          RenderMeshObject(mesh, camera);
      }
    }
  }
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
}

void GLRenderer::SetupMeshObject(const MeshObject* mesh_object) const {
  const Material& material = mesh_object->material();
  const Geometry& geometry = mesh_object->geometry();

  // Key by material.id (not texture path) — different meshes may share diffuse
  // but have unique normal/roughness maps; wrong key would cause texture aliasing.
  const std::string mat_key = "mat:" + std::to_string(material.id);
  if (!GLGlobalResources::GetInstance().textures().count(mat_key)) {
    auto ptr = std::make_unique<GLTexture>(material);
    GLGlobalResources::GetInstance().textures().emplace(mat_key, std::move(ptr));
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
  const std::string mat_key = "mat:" + std::to_string(material.id);
  const GLTexture* texture =
      GLGlobalResources::GetInstance().textures().at(mat_key).get();
  const GLBindingState* binding_state =
      GLGlobalResources::GetInstance().binding_states().at(geometry.id).get();

  const std::string shader_name = (config_ && config_->use_pbr) ? "pbr_shader" : "model_shader";
  const GLProgram* program =
      GLGlobalResources::GetInstance().programs().at(shader_name).get();

  if (config_ && config_->use_pbr)
    DrawPBR(*binding_state, *program, *texture, material, geometry.index_size());
  else
    DrawBlinnPhong(*binding_state, *program, *texture, material, geometry.index_size());
}

void GLRenderer::DrawBlinnPhong(const GLBindingState& binding_state,
                                const GLProgram& program,
                                const GLTexture& texture,
                                const Material& material,
                                unsigned int index_size) const {
  // Pass base color factor (modulates diffuse or serves as fallback color)
  program.SetVector4("base_color_factor", material.base_color);

  // Bind fallback white texture to all units first to silence driver warnings
  unsigned int fb = GLGlobalResources::GetInstance().fallback_texture();
  for (int u = 0; u < 5; ++u) {
    glActiveTexture(GL_TEXTURE0 + u);
    glBindTexture(GL_TEXTURE_2D, fb);
  }

  // unit 0: diffuse
  glActiveTexture(GL_TEXTURE0);
  if (texture.diffuse_id()) {
    glBindTexture(GL_TEXTURE_2D, texture.diffuse_id());
    program.SetInt("texture_sample", 1);
  } else {
    program.SetInt("texture_sample", 0);
  }
  program.SetInt("texture_diffuse1", 0);

  // unit 1: specular
  glActiveTexture(GL_TEXTURE1);
  if (texture.has_specular())
    glBindTexture(GL_TEXTURE_2D, texture.specular_id());
  program.SetInt("texture_specular1", 1);

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
                         const Material& material,
                         unsigned int index_size) const {
  // Bind fallback white texture to all 9 units first (silences driver warnings)
  unsigned int fb = GLGlobalResources::GetInstance().fallback_texture();
  for (int u = 0; u < 9; ++u) { glActiveTexture(GL_TEXTURE0 + u); glBindTexture(GL_TEXTURE_2D, fb); }

  // unit 0: albedo
  glActiveTexture(GL_TEXTURE0);
  program.SetInt("texture_albedo", 0);
  if (texture.diffuse_id()) {
    glBindTexture(GL_TEXTURE_2D, texture.diffuse_id());
    program.SetInt("has_albedo", 1);
  } else {
    program.SetInt("has_albedo", 0);
  }

  // unit 1: metallic-roughness (packed: G=roughness, B=metallic)
  glActiveTexture(GL_TEXTURE1);
  program.SetInt("texture_metallic_rough", 1);
  if (texture.has_metallic_roughness()) {
    glBindTexture(GL_TEXTURE_2D, texture.metallic_roughness_id());
    program.SetInt("use_metallic_rough", 1);
  } else {
    program.SetInt("use_metallic_rough", 0);
  }

  // unit 2: normal map
  glActiveTexture(GL_TEXTURE2);
  program.SetInt("texture_normal", 2);
  if (texture.has_normal()) {
    glBindTexture(GL_TEXTURE_2D, texture.normal_id());
    program.SetInt("use_normal_map", 1);
  } else {
    program.SetInt("use_normal_map", 0);
  }

  // unit 3: occlusion
  glActiveTexture(GL_TEXTURE3);
  program.SetInt("texture_occlusion", 3);
  if (texture.has_ao()) {
    glBindTexture(GL_TEXTURE_2D, texture.occlusion_id());
    program.SetInt("use_occlusion", 1);
  } else {
    program.SetInt("use_occlusion", 0);
  }

  // unit 4: emissive
  glActiveTexture(GL_TEXTURE4);
  program.SetInt("texture_emissive", 4);
  if (texture.has_emissive()) {
    glBindTexture(GL_TEXTURE_2D, texture.emissive_id());
    program.SetInt("use_emissive", 1);
  } else {
    program.SetInt("use_emissive", 0);
  }

  // unit 5: clearcoat normal
  glActiveTexture(GL_TEXTURE5);
  program.SetInt("texture_clearcoat_normal", 5);
  if (texture.has_clearcoat_normal()) {
    glBindTexture(GL_TEXTURE_2D, texture.clearcoat_normal_tex_id());
    program.SetInt("use_clearcoat_normal", 1);
  } else {
    program.SetInt("use_clearcoat_normal", 0);
  }

  // PBR factors
  program.SetVector4("base_color_factor",  material.base_color);
  program.SetFloat("metallic_factor",      material.metallic_factor);
  program.SetFloat("roughness_factor",     material.roughness_factor);
  program.SetFloat("normal_scale",         material.normal_scale);
  program.SetFloat("occlusion_strength",   material.occlusion_strength);
  program.SetVector3("emissive_factor",    material.emissive_factor);

  // KHR_materials_clearcoat uniforms
  program.SetFloat("clearcoat_factor",    material.clearcoat_factor);
  program.SetFloat("clearcoat_roughness", material.clearcoat_roughness_factor);

  // KHR_materials_specular uniforms
  program.SetFloat("specular_factor",       material.specular_factor);
  program.SetVector3("specular_color_factor", material.specular_color_factor);

  // Alpha
  program.SetInt("alpha_mode",   material.alpha_mode);
  program.SetFloat("alpha_cutoff", material.alpha_cutoff);

  // IBL textures (units 6, 7, 8)
  if (ibl_ && ibl_->loaded()) {
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ibl_->irradiance_map());
    program.SetInt("irradiance_map", 6);

    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ibl_->prefilter_map());
    program.SetInt("prefilter_map", 7);

    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_2D, ibl_->brdf_lut());
    program.SetInt("brdf_lut", 8);

    program.SetInt("use_ibl", 1);
    float ibl_intensity = config_ ? config_->ibl_intensity : 1.0f;
    program.SetFloat("ibl_intensity", ibl_intensity);
  } else {
    program.SetInt("use_ibl", 0);
  }

  // Double-sided
  if (material.double_sided) glDisable(GL_CULL_FACE);
  else                       glEnable(GL_CULL_FACE);

  glBindVertexArray(binding_state.vao());
  glDrawElements(GL_TRIANGLES, index_size, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_CULL_FACE);  // reset to no-cull (engine default)
}
