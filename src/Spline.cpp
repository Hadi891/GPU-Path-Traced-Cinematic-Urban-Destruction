#include "Spline.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

static glm::vec3 catmullRom(const glm::vec3& p0, const glm::vec3& p1,
                            const glm::vec3& p2, const glm::vec3& p3,
                            float t)
{
  float t2 = t*t;
  float t3 = t2*t;
  return 0.5f * ((2.f*p1) +
    (-p0 + p2) * t +
    (2.f*p0 - 5.f*p1 + 4.f*p2 - p3) * t2 +
    (-p0 + 3.f*p1 - 3.f*p2 + p3) * t3);
}

static glm::vec3 catmullRomTangent(const glm::vec3& p0, const glm::vec3& p1,
                                   const glm::vec3& p2, const glm::vec3& p3,
                                   float t)
{
  float t2 = t*t;
  return 0.5f * (
    (-p0 + p2) +
    2.f*(2.f*p0 - 5.f*p1 + 4.f*p2 - p3) * t +
    3.f*(-p0 + 3.f*p1 - 3.f*p2 + p3) * t2
  );
}

glm::vec3 CatmullRomSpline::sample(float t01) const {
  if (points.size() < 4) return points.empty() ? glm::vec3(0) : points.front();

  int n = (int)points.size();
  int segCount = loop ? n : (n - 3);
  float u = std::clamp(t01, 0.f, 1.f) * segCount;
  int seg = std::min((int)std::floor(u), segCount - 1);
  float t = u - seg;

  auto idx = [&](int i) {
    if (loop) return (i % n + n) % n;
    return std::clamp(i, 0, n - 1);
  };

  int i0 = loop ? seg - 1 : seg;
  int i1 = i0 + 1;
  int i2 = i0 + 2;
  int i3 = i0 + 3;

  return catmullRom(points[idx(i0)], points[idx(i1)], points[idx(i2)], points[idx(i3)], t);
}

glm::vec3 CatmullRomSpline::tangent(float t01) const {
  if (points.size() < 4) return glm::vec3(0,0,-1);

  int n = (int)points.size();
  int segCount = loop ? n : (n - 3);
  float u = std::clamp(t01, 0.f, 1.f) * segCount;
  int seg = std::min((int)std::floor(u), segCount - 1);
  float t = u - seg;

  auto idx = [&](int i) {
    if (loop) return (i % n + n) % n;
    return std::clamp(i, 0, n - 1);
  };

  int i0 = loop ? seg - 1 : seg;
  int i1 = i0 + 1;
  int i2 = i0 + 2;
  int i3 = i0 + 3;

  glm::vec3 tan = catmullRomTangent(points[idx(i0)], points[idx(i1)], points[idx(i2)], points[idx(i3)], t);
  return glm::normalize(tan);
}
