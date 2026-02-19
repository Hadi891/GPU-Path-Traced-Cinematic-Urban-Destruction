#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct GltfMeshGL {
  GLuint vao = 0;
  GLuint vbo = 0;
  GLuint ebo = 0;
  int indexCount = 0;

  GLuint albedoTex = 0;
  glm::vec4 baseColorFactor = glm::vec4(1.0f);

  std::vector<glm::vec3> cpuPos;
  std::vector<glm::vec3> cpuNrm;
  std::vector<glm::vec2> cpuUV;
  std::vector<unsigned int> cpuIdx;

  void destroy();
};

struct GltfModelGL {
  std::vector<GltfMeshGL> meshes;
  void destroy();
};

bool loadGltfModel(const std::string& path, GltfModelGL& out);
