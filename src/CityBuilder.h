#pragma once
#include <glm/glm.hpp>
#include <vector>

struct CityParams {
  int seed = 1337;

  int buildingCountPerSide = 60;
  float spacingZ = 6.0f;

  float streetHalfWidth = 8.0f;
  float baseHeight = 6.0f;
  float heightVar = 18.0f;

  float minScaleX = 6.0f;
  float maxScaleX = 10.0f;

  float minScaleZ = 6.0f;
  float maxScaleZ = 10.0f;
};

struct CityBuilder {
  CityParams P;

  std::vector<glm::mat4> buildings;

  void generate();
};
