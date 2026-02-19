#pragma once
#include <glm/glm.hpp>
#include <vector>

struct CatmullRomSpline {
  std::vector<glm::vec3> points;
  bool loop = false;

  glm::vec3 sample(float t01) const;
  glm::vec3 tangent(float t01) const;
};
