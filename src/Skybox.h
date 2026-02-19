#pragma once
#include <glad/glad.h>
#include <string>
#include <array>

struct Skybox {
  GLuint cubemap = 0;
  GLuint vao = 0;
  GLuint vbo = 0;

  bool loadCubemap(const std::array<std::string, 6>& faces, bool srgb = true);
  void createCube();
  void destroy();
};
