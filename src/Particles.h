#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class Particles {
public:
  struct GPU_Particle {
    glm::vec4 posLife;
    glm::vec4 velSeed;
  };

  int count = 0;
  GLuint ssbo = 0;

  GLuint quadVAO = 0, quadVBO = 0;
  GLuint pointVAO = 0;

  void init(int particleCount);
  void destroy();

  void dispatchCompute(GLuint computeProgram,
                       float dt, float time,
                       const glm::vec3& emitCenter,
                       const glm::vec3& emitHalfSize,
                       const glm::vec3& windDir,
                       float windAmp,
                       bool dustBurst,
                       const glm::vec3& dustCenter,
                       const glm::vec3& dustHalf,
                       uint32_t dustCount);

  void drawInstanced(GLuint renderProgram,
                     const glm::mat4& view,
                     const glm::mat4& proj,
                     float size);
  void drawPoints(GLuint renderProgram) const;
};
