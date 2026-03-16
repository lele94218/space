#include "gl_ibl.h"

#include <glad/glad.h>
#include <glog/logging.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// stb_image is compiled once in stab_image.cc; just declare the load function.
#include <stab/stab_image.h>

#include "gl_program.h"

// ──────────────────────────────────────────────────────────────────────────────
// Unit-cube geometry (for cubemap capture passes)
// ──────────────────────────────────────────────────────────────────────────────
static const float kCubeVertices[] = {
  // back face
  -1.0f, -1.0f, -1.0f,
   1.0f,  1.0f, -1.0f,
   1.0f, -1.0f, -1.0f,
   1.0f,  1.0f, -1.0f,
  -1.0f, -1.0f, -1.0f,
  -1.0f,  1.0f, -1.0f,
  // front face
  -1.0f, -1.0f,  1.0f,
   1.0f, -1.0f,  1.0f,
   1.0f,  1.0f,  1.0f,
   1.0f,  1.0f,  1.0f,
  -1.0f,  1.0f,  1.0f,
  -1.0f, -1.0f,  1.0f,
  // left face
  -1.0f,  1.0f,  1.0f,
  -1.0f,  1.0f, -1.0f,
  -1.0f, -1.0f, -1.0f,
  -1.0f, -1.0f, -1.0f,
  -1.0f, -1.0f,  1.0f,
  -1.0f,  1.0f,  1.0f,
  // right face
   1.0f,  1.0f,  1.0f,
   1.0f, -1.0f, -1.0f,
   1.0f,  1.0f, -1.0f,
   1.0f, -1.0f, -1.0f,
   1.0f,  1.0f,  1.0f,
   1.0f, -1.0f,  1.0f,
  // bottom face
  -1.0f, -1.0f, -1.0f,
   1.0f, -1.0f, -1.0f,
   1.0f, -1.0f,  1.0f,
   1.0f, -1.0f,  1.0f,
  -1.0f, -1.0f,  1.0f,
  -1.0f, -1.0f, -1.0f,
  // top face
  -1.0f,  1.0f, -1.0f,
   1.0f,  1.0f,  1.0f,
   1.0f,  1.0f, -1.0f,
   1.0f,  1.0f,  1.0f,
  -1.0f,  1.0f, -1.0f,
  -1.0f,  1.0f,  1.0f,
};

// Full-screen quad (NDC positions + UV coords, interleaved)
static const float kQuadVertices[] = {
  // positions       // tex_coords
  -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
  -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
   1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
   1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
};

// Six view matrices used to capture each face of the cubemap
static glm::mat4 CaptureViews() {
  return glm::mat4(1.0f); // not used directly — see array below
}

static glm::mat4 kCaptureViews[6] = {
  glm::lookAt(glm::vec3(0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
  glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
  glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
  glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
  glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
  glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
};

// ──────────────────────────────────────────────────────────────────────────────
GLIBL::~GLIBL() {
  if (env_cubemap_)    glDeleteTextures(1, &env_cubemap_);
  if (irradiance_map_) glDeleteTextures(1, &irradiance_map_);
  if (prefilter_map_)  glDeleteTextures(1, &prefilter_map_);
  if (brdf_lut_)       glDeleteTextures(1, &brdf_lut_);
  if (cube_vao_) { glDeleteVertexArrays(1, &cube_vao_); glDeleteBuffers(1, &cube_vbo_); }
  if (quad_vao_) { glDeleteVertexArrays(1, &quad_vao_); glDeleteBuffers(1, &quad_vbo_); }
}

// ──────────────────────────────────────────────────────────────────────────────
bool GLIBL::Load(const std::string& hdr_path) {
  // 1. Load HDR equirectangular texture
  stbi_set_flip_vertically_on_load(true);
  int width, height, nrComponents;
  float* data = stbi_loadf(hdr_path.c_str(), &width, &height, &nrComponents, 0);
  if (!data) {
    LOG(ERROR) << "[IBL] Failed to load HDR: " << hdr_path;
    return false;
  }
  LOG(ERROR) << "[IBL] Loaded HDR " << hdr_path << " (" << width << "x" << height << ")";

  unsigned int hdr_tex = 0;
  glGenTextures(1, &hdr_tex);
  glBindTexture(GL_TEXTURE_2D, hdr_tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  stbi_image_free(data);

  // 2. Convert equirectangular → cubemap
  env_cubemap_ = CreateCubemapFromEquirect(hdr_tex, 512);
  glDeleteTextures(1, &hdr_tex);

  if (!env_cubemap_) {
    LOG(ERROR) << "[IBL] Failed to create environment cubemap";
    return false;
  }

  // Generate mipmaps for the environment cubemap (used by prefilter)
  glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap_);
  glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

  // 3. Irradiance convolution
  irradiance_map_ = CreateIrradianceMap(env_cubemap_, 32);

  // 4. Prefiltered specular map
  prefilter_map_ = CreatePrefilterMap(env_cubemap_, 128);

  // 5. BRDF integration LUT
  brdf_lut_ = CreateBRDFLUT(512);

  loaded_ = (env_cubemap_ && irradiance_map_ && prefilter_map_ && brdf_lut_);
  LOG(ERROR) << "[IBL] Precomputation " << (loaded_ ? "complete" : "FAILED")
             << " env=" << env_cubemap_
             << " irr=" << irradiance_map_
             << " pre=" << prefilter_map_
             << " lut=" << brdf_lut_;
  return loaded_;
}

// ──────────────────────────────────────────────────────────────────────────────
unsigned int GLIBL::CreateCubemapFromEquirect(unsigned int hdr_tex, int size) {
  // Create FBO + RBO
  unsigned int fbo, rbo;
  glGenFramebuffers(1, &fbo);
  glGenRenderbuffers(1, &rbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glBindRenderbuffer(GL_RENDERBUFFER, rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size, size);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

  // Create cubemap texture
  unsigned int cubemap = 0;
  glGenTextures(1, &cubemap);
  glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
  for (int i = 0; i < 6; ++i)
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, size, size, 0, GL_RGB, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // Compile shader
  GLProgram prog("shaders/equirect_to_cubemap.vs", "shaders/equirect_to_cubemap.fs");

  glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

  glUseProgram(prog.program());
  prog.SetInt("equirectangular_map", 0);
  prog.SetMatrix4("projection", glm::value_ptr(proj));
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, hdr_tex);

  glViewport(0, 0, size, size);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  for (int i = 0; i < 6; ++i) {
    prog.SetMatrix4("view", glm::value_ptr(kCaptureViews[i]));
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemap, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    RenderCube();
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDeleteFramebuffers(1, &fbo);
  glDeleteRenderbuffers(1, &rbo);

  return cubemap;
}

// ──────────────────────────────────────────────────────────────────────────────
unsigned int GLIBL::CreateIrradianceMap(unsigned int env_cubemap, int size) {
  unsigned int fbo, rbo;
  glGenFramebuffers(1, &fbo);
  glGenRenderbuffers(1, &rbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glBindRenderbuffer(GL_RENDERBUFFER, rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size, size);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

  unsigned int irr_map = 0;
  glGenTextures(1, &irr_map);
  glBindTexture(GL_TEXTURE_CUBE_MAP, irr_map);
  for (int i = 0; i < 6; ++i)
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, size, size, 0, GL_RGB, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  GLProgram prog("shaders/irradiance_convolution.vs", "shaders/irradiance_convolution.fs");
  glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

  glUseProgram(prog.program());
  prog.SetInt("environment_map", 0);
  prog.SetMatrix4("projection", glm::value_ptr(proj));
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap);

  glViewport(0, 0, size, size);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  for (int i = 0; i < 6; ++i) {
    prog.SetMatrix4("view", glm::value_ptr(kCaptureViews[i]));
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irr_map, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    RenderCube();
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDeleteFramebuffers(1, &fbo);
  glDeleteRenderbuffers(1, &rbo);

  return irr_map;
}

// ──────────────────────────────────────────────────────────────────────────────
unsigned int GLIBL::CreatePrefilterMap(unsigned int env_cubemap, int size) {
  unsigned int prefilter = 0;
  glGenTextures(1, &prefilter);
  glBindTexture(GL_TEXTURE_CUBE_MAP, prefilter);
  for (int i = 0; i < 6; ++i)
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, size, size, 0, GL_RGB, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

  GLProgram prog("shaders/prefilter.vs", "shaders/prefilter.fs");
  glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

  glUseProgram(prog.program());
  prog.SetInt("environment_map", 0);
  prog.SetMatrix4("projection", glm::value_ptr(proj));
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap);

  unsigned int fbo, rbo;
  glGenFramebuffers(1, &fbo);
  glGenRenderbuffers(1, &rbo);

  const int kMaxMipLevels = 5;
  for (int mip = 0; mip < kMaxMipLevels; ++mip) {
    int mip_size = size >> mip;  // 128, 64, 32, 16, 8
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mip_size, mip_size);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    float roughness = (float)mip / (float)(kMaxMipLevels - 1);
    prog.SetFloat("roughness", roughness);

    glViewport(0, 0, mip_size, mip_size);
    for (int i = 0; i < 6; ++i) {
      prog.SetMatrix4("view", glm::value_ptr(kCaptureViews[i]));
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilter, mip);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      RenderCube();
    }
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDeleteFramebuffers(1, &fbo);
  glDeleteRenderbuffers(1, &rbo);

  return prefilter;
}

// ──────────────────────────────────────────────────────────────────────────────
unsigned int GLIBL::CreateBRDFLUT(int size) {
  unsigned int lut = 0;
  glGenTextures(1, &lut);
  glBindTexture(GL_TEXTURE_2D, lut);
  // RG16F: stores (scale, bias) for specular BRDF integration
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, size, size, 0, GL_RG, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  unsigned int fbo, rbo;
  glGenFramebuffers(1, &fbo);
  glGenRenderbuffers(1, &rbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glBindRenderbuffer(GL_RENDERBUFFER, rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size, size);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lut, 0);

  GLProgram prog("shaders/brdf_lut.vs", "shaders/brdf_lut.fs");
  glUseProgram(prog.program());

  glViewport(0, 0, size, size);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  RenderQuad();

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDeleteFramebuffers(1, &fbo);
  glDeleteRenderbuffers(1, &rbo);

  return lut;
}

// ──────────────────────────────────────────────────────────────────────────────
void GLIBL::RenderCube() {
  if (!cube_vao_) {
    glGenVertexArrays(1, &cube_vao_);
    glGenBuffers(1, &cube_vbo_);
    glBindVertexArray(cube_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kCubeVertices), kCubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
  }
  glBindVertexArray(cube_vao_);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);
}

// ──────────────────────────────────────────────────────────────────────────────
void GLIBL::RenderQuad() {
  if (!quad_vao_) {
    glGenVertexArrays(1, &quad_vao_);
    glGenBuffers(1, &quad_vbo_);
    glBindVertexArray(quad_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kQuadVertices), kQuadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);
  }
  glBindVertexArray(quad_vao_);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindVertexArray(0);
}
