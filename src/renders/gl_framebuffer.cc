#include "gl_framebuffer.h"

#include <glad/glad.h>
#include <glog/logging.h>

GLFramebuffer::GLFramebuffer(int width, int height, int samples)
    : width_(width), height_(height), samples_(samples) {
  Create();
}

GLFramebuffer::~GLFramebuffer() {
  Destroy();
}

void GLFramebuffer::Create() {
  // Clamp samples to driver maximum
  int max_samples = 0;
  glGetIntegerv(GL_MAX_SAMPLES, &max_samples);
  if (samples_ > max_samples) {
    LOG(ERROR) << "Requested " << samples_ << "x MSAA, driver max is "
               << max_samples << "x. Clamping.";
    samples_ = max_samples;
  }

  // --- MSAA FBO ---
  glGenFramebuffers(1, &msaa_fbo_);
  glBindFramebuffer(GL_FRAMEBUFFER, msaa_fbo_);

  // Multisample color renderbuffer
  glGenRenderbuffers(1, &msaa_color_rbo_);
  glBindRenderbuffer(GL_RENDERBUFFER, msaa_color_rbo_);
  glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples_, GL_RGBA8, width_, height_);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaa_color_rbo_);

  // Multisample depth renderbuffer
  glGenRenderbuffers(1, &msaa_depth_rbo_);
  glBindRenderbuffer(GL_RENDERBUFFER, msaa_depth_rbo_);
  glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples_, GL_DEPTH24_STENCIL8, width_, height_);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, msaa_depth_rbo_);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    LOG(ERROR) << "MSAA framebuffer is not complete!";

  // --- Resolve FBO (plain texture for blit target) ---
  glGenFramebuffers(1, &resolve_fbo_);
  glBindFramebuffer(GL_FRAMEBUFFER, resolve_fbo_);

  glGenTextures(1, &resolved_color_tex_);
  glBindTexture(GL_TEXTURE_2D, resolved_color_tex_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resolved_color_tex_, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    LOG(ERROR) << "Resolve framebuffer is not complete!";

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLFramebuffer::Destroy() {
  glDeleteRenderbuffers(1, &msaa_color_rbo_);
  glDeleteRenderbuffers(1, &msaa_depth_rbo_);
  glDeleteFramebuffers(1, &msaa_fbo_);
  glDeleteTextures(1, &resolved_color_tex_);
  glDeleteFramebuffers(1, &resolve_fbo_);
  msaa_fbo_ = msaa_color_rbo_ = msaa_depth_rbo_ = 0;
  resolve_fbo_ = resolved_color_tex_ = 0;
}

void GLFramebuffer::Bind() const {
  glBindFramebuffer(GL_FRAMEBUFFER, msaa_fbo_);
  glViewport(0, 0, width_, height_);
}

void GLFramebuffer::BindDefault() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLFramebuffer::Resolve() const {
  glBindFramebuffer(GL_READ_FRAMEBUFFER, msaa_fbo_);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolve_fbo_);
  glBlitFramebuffer(0, 0, width_, height_,
                    0, 0, width_, height_,
                    GL_COLOR_BUFFER_BIT, GL_NEAREST);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLFramebuffer::Rebuild(int width, int height, int samples) {
  Destroy();
  width_ = width;
  height_ = height;
  samples_ = samples;
  Create();
}
