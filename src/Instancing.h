#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

struct InstanceBuffer {
  GLuint buffer = 0;
  int capacity = 0;

  void create();
  void destroy();

  void upload(const std::vector<glm::mat4>& models);

  void bindToVAO(GLuint vao, int firstLocation = 2);
};
