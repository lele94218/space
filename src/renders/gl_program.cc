#include "gl_program.h"

#include <glog/logging.h>

#include <fstream>
#include <iostream>
#include <sstream>

GLShader::GLShader(GLenum shader_type, const std::string& shader_source) {
  const unsigned int shader = glCreateShader(shader_type);
  const char* source = shader_source.c_str();
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);
  int success;
  char message[512];
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shader, 512, nullptr, message);
    LOG(ERROR) << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << message;
  }
  CHECK(shader) << "Failded to create shader.";
  shader_ = shader;
}

GLShader::~GLShader() { glDeleteShader(shader_); }

GLProgram::GLProgram(const std::string& vertex_shader_path,
                     const std::string& fragment_shader_path) {
  const unsigned int program = glCreateProgram();
  CHECK(program) << "Failed to create program from vertex shader: " << vertex_shader_path
                 << " and fragment shader: " << fragment_shader_path;
  vertex_shader_source_ = LoadShaderFile(vertex_shader_path);
  fragment_shader_source_ = LoadShaderFile(fragment_shader_path);
  GLShader vertex_shader(GL_VERTEX_SHADER, vertex_shader_source_);
  GLShader fragment_shader(GL_FRAGMENT_SHADER, fragment_shader_source_);

  glAttachShader(program, vertex_shader.shader());
  glAttachShader(program, fragment_shader.shader());
  glLinkProgram(program);
  int success;
  char message[512];
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(program, 512, nullptr, message);
    LOG(ERROR) << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << message;
  }
  program_ = program;
}

GLProgram::~GLProgram() { glDeleteProgram(program_); }

std::string GLProgram::LoadShaderFile(const std::string& file_path) {
  std::ifstream shader_file;
  std::stringstream shader_stream;
  shader_file.open(file_path);
  shader_stream << shader_file.rdbuf();
  shader_file.close();
  return shader_stream.str();
}

void GLProgram::SetBool(const std::string& name, bool value) const {
  glUniform1i(glGetUniformLocation(program_, name.c_str()), (int)value);
}

void GLProgram::SetInt(const std::string& name, int value) const {
  glUniform1i(glGetUniformLocation(program_, name.c_str()), value);
}

void GLProgram::SetFloat(const std::string& name, float value) const {
  glUniform1f(glGetUniformLocation(program_, name.c_str()), value);
}

void GLProgram::SetMatrix4(const std::string& name, const float* value) const {
  glUniformMatrix4fv(glGetUniformLocation(program_, name.c_str()), 1, GL_FALSE, value);
}

void GLProgram::SetVector3(const std::string& name, const float* value) const {
  glUniform3fv(glGetUniformLocation(program_, name.c_str()), 1, value);
}

unsigned int GLProgram::GetAttributeLocation(const std::string& name) const {
  return glGetAttribLocation(program_, name.c_str());
}
