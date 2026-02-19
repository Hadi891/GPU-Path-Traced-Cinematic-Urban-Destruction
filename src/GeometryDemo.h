#pragma once
#include <vector>
#include <glm/glm.hpp>

struct GeometryDemo {
  std::vector<glm::vec3> basePos;
  std::vector<glm::vec3> pos;
  std::vector<glm::uvec3> tri;
  std::vector<std::vector<uint32_t>> nbr;
  std::vector<glm::vec3> col;

  unsigned vao = 0, vboPos = 0, vboCol = 0, ebo = 0;
  int indexCount = 0;

  int mode = 1;
  int iters = 10;
  float lambda = 0.35f;
  float mu = -0.34f;
  
  bool ready = false;

  void buildFromIndexed(const std::vector<glm::vec3>& positions,
                        const std::vector<glm::uvec3>& tris);

  void rebuildAdjacency();
  void resetToBase();

  void applyLaplacian(int iterations, float lam);
  void applyTaubin(int iterations, float lam, float mu);
  void computeCurvatureColors();

  void setSolidColor(const glm::vec3& c);
  void uploadToGPU();
  void draw() const;

  void destroy();
};
