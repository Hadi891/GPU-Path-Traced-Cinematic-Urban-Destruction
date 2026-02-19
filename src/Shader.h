#pragma once
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Shader {
public:
  Shader() = default;
  ~Shader();

  bool loadGraphics(const std::string& vertPath, const std::string& fragPath);
  bool loadCompute(const std::string& compPath);

  void use() const;
  GLuint id() const { return m_program; }

  void setMat4(const char* name, const float* value) const;
  void setVec3(const char* name, float x, float y, float z) const;
  void setFloat(const char* name, float v) const;

private:
  GLuint m_program = 0;

  static GLuint compile(GLenum type, const std::string& src, const std::string& debugName);
  static bool link(GLuint program, const std::string& debugName);
};
