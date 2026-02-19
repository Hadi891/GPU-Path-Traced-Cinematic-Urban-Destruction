#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

struct RTTriangle {
  glm::vec4 v0;
  glm::vec4 v1;
  glm::vec4 v2;

  glm::vec4 n0;
  glm::vec4 n1;
  glm::vec4 n2;

  glm::vec4 uv0;
  glm::vec4 uv1;
  glm::vec4 uv2;

  glm::ivec4 meta;
};

struct BVHNode {
  glm::vec4 bmin;
  glm::vec4 bmax;
  int leftChild;
  int rightChild;
  int leftFirst;
  int triCount;
};

struct RaytraceSceneGL {
  GLuint ssboTris = 0;
  GLuint ssboNodes = 0;
  int triCount = 0;
  int nodeCount = 0;

  void destroy();
};

void buildAndUploadBVH(RaytraceSceneGL& rt, std::vector<RTTriangle>& tris);