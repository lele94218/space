#pragma once

class GLFramebuffer {
 public:
  GLFramebuffer(int width, int height, int samples);
  ~GLFramebuffer();

  // Disable copy
  GLFramebuffer(const GLFramebuffer&) = delete;
  GLFramebuffer& operator=(const GLFramebuffer&) = delete;

  // Bind this FBO as render target
  void Bind() const;

  // Bind default framebuffer (screen)
  static void BindDefault();

  // Blit MSAA FBO → resolve FBO (must call before using resolved_texture_id)
  void Resolve() const;

  // Rebuild with new size or sample count
  void Rebuild(int width, int height, int samples);

  int width() const { return width_; }
  int height() const { return height_; }
  int samples() const { return samples_; }
  unsigned int resolved_texture_id() const { return resolved_color_tex_; }
  unsigned int resolve_fbo_pub() const { return resolve_fbo_; }

 private:
  void Create();
  void Destroy();

  int width_;
  int height_;
  int samples_;

  // MSAA FBO (render target)
  unsigned int msaa_fbo_ = 0;
  unsigned int msaa_color_rbo_ = 0;  // multisample color renderbuffer
  unsigned int msaa_depth_rbo_ = 0;  // multisample depth renderbuffer

  // Resolve FBO (plain texture, for post-processing or direct blit)
  unsigned int resolve_fbo_ = 0;
  unsigned int resolved_color_tex_ = 0;
};
