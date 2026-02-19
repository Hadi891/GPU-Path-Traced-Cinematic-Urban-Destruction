#include "CityBuilder.h"
#include <glm/gtc/matrix_transform.hpp>
#include <random>

void CityBuilder::generate() {
  buildings.clear();

  std::mt19937 rng((unsigned)P.seed);
  std::uniform_real_distribution<float> u01(0.f, 1.f);

  auto randRange = [&](float a, float b) {
    return a + (b - a) * u01(rng);
  };

  for (int side = 0; side < 2; ++side) {
    float xBase = (side == 0) ? -P.streetHalfWidth : +P.streetHalfWidth;

    for (int i = 0; i < P.buildingCountPerSide; ++i) {
      float z = -i * P.spacingZ;

      float sx = randRange(P.minScaleX, P.maxScaleX);
      float sz = randRange(P.minScaleZ, P.maxScaleZ);
      float h  = P.baseHeight + randRange(0.f, P.heightVar);

      float xJitter = randRange(-1.0f, 1.0f);
      float zJitter = randRange(-0.8f, 0.8f);

      glm::mat4 M(1.0f);
      M = glm::translate(M, glm::vec3(xBase + xJitter, h * 0.5f, z + zJitter));
      M = glm::scale(M, glm::vec3(sx, h, sz));

      buildings.push_back(M);
    }
  }
}
