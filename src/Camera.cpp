#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/compatibility.hpp>
#include <cmath>

static float rad(float deg) { return deg * 3.1415926535f / 180.f; }

glm::vec3 Camera::forward() const {
  float yaw = rad(yawDeg);
  float pitch = rad(pitchDeg);
  glm::vec3 f;
  f.x = std::cos(yaw) * std::cos(pitch);
  f.y = std::sin(pitch);
  f.z = std::sin(yaw) * std::cos(pitch);
  return glm::normalize(f);
}

glm::vec3 Camera::right() const {
  return glm::normalize(glm::cross(forward(), glm::vec3(0,1,0)));
}

glm::mat4 Camera::view() const {
  return glm::lookAt(pos, pos + forward(), glm::vec3(0,1,0));
}

glm::mat4 Camera::proj(float aspect) const {
  return glm::perspective(glm::radians(fovDeg), aspect, nearZ, farZ);
}
