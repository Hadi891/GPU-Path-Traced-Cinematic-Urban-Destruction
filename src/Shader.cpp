#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Shader.h"
#include "gl_utils.h"
#include <iostream>

Shader::~Shader() {
  if (m_program) glDeleteProgram(m_program);
}

GLuint Shader::compile(GLenum type, const std::string& src, const std::string& debugName) {
  GLuint sh = glCreateShader(type);
  const char* csrc = src.c_str();
  glShaderSource(sh, 1, &csrc, nullptr);
  glCompileShader(sh);

  GLint ok = 0;
  glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    GLint len = 0;
    glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
    std::string log((size_t)len, '\0');
    glGetShaderInfoLog(sh, len, nullptr, &log[0]);
    std::cerr << "[SHADER COMPILE FAIL] " << debugName << "\n" << log << "\n";
    glDeleteShader(sh);
    return 0;
  }
  return sh;
}

bool Shader::link(GLuint program, const std::string& debugName) {
  glLinkProgram(program);
  GLint ok = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &ok);
  if (!ok) {
    GLint len = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
    std::string log((size_t)len, '\0');
    glGetProgramInfoLog(program, len, nullptr, &log[0]);
    std::cerr << "[PROGRAM LINK FAIL] " << debugName << "\n" << log << "\n";
    return false;
  }
  return true;
}

bool Shader::loadGraphics(const std::string& vertPath, const std::string& fragPath) {
  std::string vsrc, fsrc;
  if (!loadTextFile(vertPath, vsrc)) { std::cerr << "Missing " << vertPath << "\n"; return false; }
  if (!loadTextFile(fragPath, fsrc)) { std::cerr << "Missing " << fragPath << "\n"; return false; }

  GLuint vs = compile(GL_VERTEX_SHADER, vsrc, vertPath);
  GLuint fs = compile(GL_FRAGMENT_SHADER, fsrc, fragPath);
  if (!vs || !fs) return false;

  if (m_program) glDeleteProgram(m_program);
  m_program = glCreateProgram();
  glAttachShader(m_program, vs);
  glAttachShader(m_program, fs);
  bool ok = link(m_program, "graphics");

  glDeleteShader(vs);
  glDeleteShader(fs);
  return ok;
}

bool Shader::loadCompute(const std::string& compPath) {
  std::string csrc;
  if (!loadTextFile(compPath, csrc)) { std::cerr << "Missing " << compPath << "\n"; return false; }
  GLuint cs = compile(GL_COMPUTE_SHADER, csrc, compPath);
  if (!cs) return false;

  if (m_program) glDeleteProgram(m_program);
  m_program = glCreateProgram();
  glAttachShader(m_program, cs);
  bool ok = link(m_program, "compute");
  glDeleteShader(cs);
  return ok;
}

void Shader::use() const { glUseProgram(m_program); }

void Shader::setMat4(const char* name, const float* value) const {
  GLint loc = glGetUniformLocation(m_program, name);
  if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, value);
}
void Shader::setVec3(const char* name, float x, float y, float z) const {
  GLint loc = glGetUniformLocation(m_program, name);
  if (loc >= 0) glUniform3f(loc, x, y, z);
}
void Shader::setFloat(const char* name, float v) const {
  GLint loc = glGetUniformLocation(m_program, name);
  if (loc >= 0) glUniform1f(loc, v);
}
