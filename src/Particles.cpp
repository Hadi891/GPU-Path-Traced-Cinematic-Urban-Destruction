#include "Particles.h"
#include <glm/gtc/type_ptr.hpp>
#include <random>

static void set1f(GLuint p, const char* n, float v) {
  GLint loc = glGetUniformLocation(p, n);
  if (loc >= 0) glUniform1f(loc, v);
}
static void set3f(GLuint p, const char* n, const glm::vec3& v) {
  GLint loc = glGetUniformLocation(p, n);
  if (loc >= 0) glUniform3f(loc, v.x, v.y, v.z);
}
static void setMat4(GLuint p, const char* n, const glm::mat4& m) {
  GLint loc = glGetUniformLocation(p, n);
  if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(m));
}

static void set1i(GLuint p, const char* n, int v) {
  GLint loc = glGetUniformLocation(p, n);
  if (loc >= 0) glUniform1i(loc, v);
}

void Particles::init(int particleCount) {
  count = particleCount;

  std::vector<GPU_Particle> initData(count);
  std::mt19937 rng(1337);
  std::uniform_real_distribution<float> u01(0.f, 1.f);

  for (int i = 0; i < count; ++i) {
    initData[i].posLife = glm::vec4(0,0,0, -1.f);
    initData[i].velSeed = glm::vec4(0,0,0, u01(rng) * 10000.f + 1.f);
  }

  glGenBuffers(1, &ssbo);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
  glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)(initData.size() * sizeof(GPU_Particle)),
               initData.data(), GL_DYNAMIC_DRAW);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  float quad[] = {
    -0.5f, -0.5f,
     0.5f, -0.5f,
     0.5f,  0.5f,
    -0.5f,  0.5f
  };

  glGenVertexArrays(1, &quadVAO);
  glGenBuffers(1, &quadVBO);
  glGenVertexArrays(1, &pointVAO);

  glBindVertexArray(quadVAO);
  glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

  glBindVertexArray(0);
}

void Particles::destroy() {
  if (quadVBO) glDeleteBuffers(1, &quadVBO);
  if (quadVAO) glDeleteVertexArrays(1, &quadVAO);
  if (ssbo) glDeleteBuffers(1, &ssbo);
  if (pointVAO) glDeleteVertexArrays(1, &pointVAO);
  pointVAO = 0;
  quadVBO = quadVAO = ssbo = 0;
  count = 0;
}

void Particles::dispatchCompute(GLuint computeProgram,
                                float dt, float time,
                                const glm::vec3& emitCenter,
                                const glm::vec3& emitHalfSize,
                                const glm::vec3& windDir,
                                float windAmp,
                                bool dustBurst,
                                const glm::vec3& dustCenter,
                                const glm::vec3& dustHalf,
                                uint32_t dustCount)
{
  glUseProgram(computeProgram);
  glUniform1ui(glGetUniformLocation(computeProgram, "uDustCount"), (GLuint)dustCount);

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

  set1f(computeProgram, "uDt", dt);
  set1f(computeProgram, "uTime", time);
  set3f(computeProgram, "uEmitCenter", emitCenter);
  set3f(computeProgram, "uEmitHalfSize", emitHalfSize);
  set3f(computeProgram, "uWindDir", windDir);
  set1f(computeProgram, "uWindAmp", windAmp);
  set1i(computeProgram, "uDustBurst", dustBurst ? 1 : 0);
  set3f(computeProgram, "uDustCenter", dustCenter);
  set3f(computeProgram, "uDustHalf",   dustHalf);

  GLuint groups = (GLuint)((count + 255) / 256);
  glDispatchCompute(groups, 1, 1);

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
}

void Particles::drawInstanced(GLuint renderProgram,
                              const glm::mat4& view,
                              const glm::mat4& proj,
                              float size)
{
  glUseProgram(renderProgram);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

  setMat4(renderProgram, "uView", view);
  setMat4(renderProgram, "uProj", proj);

  GLint locSize = glGetUniformLocation(renderProgram, "uSize");
  if (locSize >= 0) glUniform1f(locSize, size);

  glBindVertexArray(quadVAO);
  glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, count);
  glBindVertexArray(0);
}

void Particles::drawPoints(GLuint renderProgram) const {
  glUseProgram(renderProgram);

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

  glBindVertexArray(pointVAO);
  glDrawArrays(GL_POINTS, 0, count);
  glBindVertexArray(0);
}

