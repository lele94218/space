#pragma once

#include <string>

class Shader {
 public:
  // the program ID
  unsigned int ID;

  // constructor reads and builds the shader
  Shader(const char* vertexPath, const char* fragmentPath);
  // use/activate the shader
  void use() const;
  void reset() const;
  // utility uniform functions
  void setBool(const std::string& name, bool value) const;
  void setInt(const std::string& name, int value) const;
  void setFloat(const std::string& name, float value) const;

 private:
  unsigned int LoadVertaxShader(const char* vertexShaderSource) const;
  unsigned int LoadFragmentShader(const char* fragmentShaderSource) const;
  void LinkShaderProgram(unsigned int vertexShader, unsigned int fragmentShader);
};
