#pragma once

#include <glad/glad.h>

#include <string>

class GLShader {
 public:
  GLShader(unsigned int shader_type, const std::string& shader_source);
  ~GLShader();

  unsigned int shader() const { return shader_; }

 private:
  unsigned int shader_;
};

class GLProgram {
 public:
  GLProgram(const std::string& vertex_shader_path, const std::string& fragment_shader_path);
  ~GLProgram();

  unsigned int program() const { return program_; }

  void SetBool(const std::string& name, bool value) const;
  void SetInt(const std::string& name, int value) const;
  void SetFloat(const std::string& name, float value) const;
  void SetMatrix4(const std::string& name, const float* value) const;
  void SetVector3(const std::string& name, const float* value) const;
  unsigned int GetAttributeLocation(const std::string& name) const;

 private:
  std::string LoadShaderFile(const std::string& file_path);

  std::string vertex_shader_source_;
  std::string fragment_shader_source_;
  unsigned int program_;
};
