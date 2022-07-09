#include "shader.h"

#include <glad/glad.h>
#include <glog/logging.h>

#include <fstream>
#include <iostream>
#include <sstream>

unsigned int Shader::LoadVertaxShader(const char* vertexShaderSource) const {
  unsigned int vertexShader;
  vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);
  int success;
  char infoLog[512];
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    LOG(ERROR) << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog;
  }
  return vertexShader;
}

unsigned int Shader::LoadFragmentShader(const char* fragmentShaderSource) const {
  unsigned int fragmentShader;
  fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);
  int success;
  char infoLog[512];
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
    LOG(ERROR) << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog;
  }
  return fragmentShader;
}

void Shader::LinkShaderProgram(unsigned int vertexShader, unsigned int fragmentShader) {
  unsigned int shaderProgram;
  shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  int success;
  char infoLog[512];
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    LOG(ERROR) << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog;
  }
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
  ID = shaderProgram;
}

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
  // 1. retrieve the vertex/fragment source code from filePath
  std::string vertexCode;
  std::string fragmentCode;
  std::ifstream vShaderFile;
  std::ifstream fShaderFile;
  // ensure ifstream objects can throw exceptions:
  vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  try {
    // open files
    vShaderFile.open(vertexPath);
    fShaderFile.open(fragmentPath);
    std::stringstream vShaderStream, fShaderStream;
    // read file's buffer contents into streams
    vShaderStream << vShaderFile.rdbuf();
    fShaderStream << fShaderFile.rdbuf();
    // close file handlers
    vShaderFile.close();
    fShaderFile.close();
    // convert stream into string
    vertexCode = vShaderStream.str();
    fragmentCode = fShaderStream.str();
  } catch (std::ifstream::failure e) {
    LOG(ERROR) << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ";
  }
  const unsigned int vertexShader = LoadVertaxShader(vertexCode.c_str());
  const unsigned int fragmentShader = LoadFragmentShader(fragmentCode.c_str());
  LinkShaderProgram(vertexShader, fragmentShader);
}

void Shader::use() const { glUseProgram(ID); }

void Shader::reset() const { glDeleteProgram(ID); }

void Shader::setBool(const std::string& name, bool value) const {
  glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Shader::setInt(const std::string& name, int value) const {
  glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float value) const {
  glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setMatrix4(const std::string& name, const float* value) const {
  glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, value);
}

void Shader::setVector3(const std::string& name, const float* value) const {
  glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, value);
}
