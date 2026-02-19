#pragma once
#include <glad/glad.h>

struct Offscreen {
  int w = 0, h = 0;
  GLuint fbo = 0;
  GLuint colorTex = 0;
  GLuint depthTex = 0;

  void create(int width, int height);
  void destroy();
  void bind();
  void unbind();
};

struct FullscreenQuad {
  GLuint vao = 0, vbo = 0;

  void create();
  void destroy();
  void draw();
};
