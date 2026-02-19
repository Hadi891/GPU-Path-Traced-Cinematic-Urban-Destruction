#include "ShadowMap.h"
#include <iostream>

void ShadowMap::create(int shadowSize) {
  if (fbo && size == shadowSize) return;
  destroy();
  size = shadowSize;

  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  glGenTextures(1, &depthTex);
  glBindTexture(GL_TEXTURE_2D, depthTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, size, size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  float border[4] = {1.f, 1.f, 1.f, 1.f};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);

  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "ShadowMap FBO incomplete!\n";
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMap::destroy() {
  if (depthTex) glDeleteTextures(1, &depthTex);
  if (fbo) glDeleteFramebuffers(1, &fbo);
  depthTex = fbo = 0;
}

void ShadowMap::begin() {
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glViewport(0, 0, size, size);
  glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::end(int w, int h) {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, w, h);
}
