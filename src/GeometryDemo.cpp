#include "GeometryDemo.h"
#include <glad/glad.h>
#include <algorithm>
#include <cmath>

static glm::vec3 heat(float t){
  t = glm::clamp(t, 0.0f, 1.0f);
  glm::vec3 c;
  if (t < 0.33f) {
    float a = t / 0.33f;
    c = glm::mix(glm::vec3(0,0,1), glm::vec3(0,1,1), a);
  } else if (t < 0.66f) {
    float a = (t - 0.33f) / 0.33f;
    c = glm::mix(glm::vec3(0,1,1), glm::vec3(1,1,0), a);
  } else {
    float a = (t - 0.66f) / 0.34f;
    c = glm::mix(glm::vec3(1,1,0), glm::vec3(1,0,0), a);
  }
  return c;
}

void GeometryDemo::buildFromIndexed(const std::vector<glm::vec3>& positions,
                                    const std::vector<glm::uvec3>& tris)
{
  basePos = positions;
  pos = positions;
  tri = tris;
  indexCount = (int)tri.size() * 3;

  col.assign(pos.size(), glm::vec3(0.8f));

  rebuildAdjacency();

  if (!vao) glGenVertexArrays(1, &vao);
  if (!vboPos) glGenBuffers(1, &vboPos);
  if (!vboCol) glGenBuffers(1, &vboCol);
  if (!ebo) glGenBuffers(1, &ebo);

  uploadToGPU();
  ready = true;
}

void GeometryDemo::rebuildAdjacency(){
  nbr.assign(basePos.size(), {});
  nbr.shrink_to_fit();
  nbr.assign(basePos.size(), {});
  auto addEdge = [&](uint32_t a, uint32_t b){
    auto& A = nbr[a];
    if (std::find(A.begin(), A.end(), b) == A.end()) A.push_back(b);
  };
  for (auto t : tri){
    uint32_t a=t.x, b=t.y, c=t.z;
    addEdge(a,b); addEdge(a,c);
    addEdge(b,a); addEdge(b,c);
    addEdge(c,a); addEdge(c,b);
  }
}

void GeometryDemo::resetToBase(){
  pos = basePos;
}

static void laplacianOnce(const std::vector<glm::vec3>& inPos,
                          std::vector<glm::vec3>& outPos,
                          const std::vector<std::vector<uint32_t>>& nbr,
                          float lam)
{
  outPos = inPos;
  for (size_t i=0;i<inPos.size();++i){
    const auto& N = nbr[i];
    if (N.empty()) continue;
    glm::vec3 avg(0.f);
    for (uint32_t j : N) avg += inPos[j];
    avg /= (float)N.size();
    glm::vec3 L = avg - inPos[i];
    outPos[i] = inPos[i] + lam * L;
  }
}

void GeometryDemo::applyLaplacian(int iterations, float lam){
  std::vector<glm::vec3> tmp;
  for (int k=0;k<iterations;k++){
    laplacianOnce(pos, tmp, nbr, lam);
    pos.swap(tmp);
  }
}

void GeometryDemo::applyTaubin(int iterations, float lam, float mu_){
  std::vector<glm::vec3> tmp;
  for (int k=0;k<iterations;k++){
    laplacianOnce(pos, tmp, nbr, lam);
    pos.swap(tmp);
    laplacianOnce(pos, tmp, nbr, mu_);
    pos.swap(tmp);
  }
}

void GeometryDemo::setSolidColor(const glm::vec3& c){
  col.assign(pos.size(), c);
}

void GeometryDemo::computeCurvatureColors(){
  std::vector<float> mag(pos.size(), 0.f);
  float mx = 1e-8f;

  for (size_t i=0;i<pos.size();++i){
    const auto& N = nbr[i];
    if (N.empty()) { mag[i]=0.f; continue; }
    glm::vec3 avg(0.f);
    for (uint32_t j : N) avg += pos[j];
    avg /= (float)N.size();
    glm::vec3 L = avg - pos[i];
    float m = glm::length(L);
    mag[i]=m;
    mx = std::max(mx, m);
  }

  col.resize(pos.size());
  for (size_t i=0;i<pos.size();++i){
    float t = mag[i] / mx;
    t = std::pow(t, 0.6f);
    col[i] = heat(t);
  }
}

void GeometryDemo::uploadToGPU(){
  glBindVertexArray(vao);

  glBindBuffer(GL_ARRAY_BUFFER, vboPos);
  glBufferData(GL_ARRAY_BUFFER, pos.size()*sizeof(glm::vec3), pos.data(), GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

  glBindBuffer(GL_ARRAY_BUFFER, vboCol);
  glBufferData(GL_ARRAY_BUFFER, col.size()*sizeof(glm::vec3), col.data(), GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, tri.size()*sizeof(glm::uvec3), tri.data(), GL_STATIC_DRAW);

  glBindVertexArray(0);
}

void GeometryDemo::draw() const{
  if (!ready) return;
  glBindVertexArray(vao);
  glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}

void GeometryDemo::destroy(){
  if (vboPos) glDeleteBuffers(1,&vboPos);
  if (vboCol) glDeleteBuffers(1,&vboCol);
  if (ebo) glDeleteBuffers(1,&ebo);
  if (vao) glDeleteVertexArrays(1,&vao);
  vao=vboPos=vboCol=ebo=0;
  ready=false;
}
