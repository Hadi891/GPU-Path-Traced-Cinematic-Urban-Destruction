#pragma once
#include <glm/glm.hpp>

struct Camera {
  glm::vec3 pos   = glm::vec3(0.f, 1.6f, 6.f);
  float yawDeg    = -90.f;
  float pitchDeg  = 0.f;

  float fovDeg    = 60.f;
  float nearZ     = 0.1f;
  float farZ      = 500.f;

  glm::mat4 view() const;
  glm::mat4 proj(float aspect) const;

  glm::vec3 forward() const;
  glm::vec3 right() const;
};
