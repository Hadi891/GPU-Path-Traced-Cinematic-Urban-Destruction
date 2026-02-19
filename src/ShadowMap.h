#pragma once
#include <glad/glad.h>

struct ShadowMap {
  int size = 2048;
  GLuint fbo = 0;
  GLuint depthTex = 0;

  void create(int shadowSize);
  void destroy();

  void begin();
  void end(int w, int h);
};
