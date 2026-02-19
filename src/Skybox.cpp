#include "Skybox.h"
#include <iostream>

#include "stb_image.h"


bool Skybox::loadCubemap(const std::array<std::string, 6>& faces, bool srgb) {
  if (!cubemap) glGenTextures(1, &cubemap);
  glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);

  stbi_set_flip_vertically_on_load(false);

  for (int i = 0; i < 6; i++) {
    int w, h, n;
    unsigned char* data = stbi_load(faces[i].c_str(), &w, &h, &n, 0);
    if (!data) {
      std::cerr << "Failed to load cubemap face: " << faces[i] << "\n";
      return false;
    }

    GLenum format = (n == 4) ? GL_RGBA : GL_RGB;

    GLenum internalFormat = srgb
      ? ((n == 4) ? GL_SRGB8_ALPHA8 : GL_SRGB8)
      : ((n == 4) ? GL_RGBA8 : GL_RGB8);

    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                 0, internalFormat, w, h, 0, format, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
  }

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  return true;
}

void Skybox::createCube() {
  float verts[] = {
    -1,-1,-1,  1,-1,-1,  1, 1,-1,  1, 1,-1, -1, 1,-1, -1,-1,-1,
    -1,-1, 1,  1,-1, 1,  1, 1, 1,  1, 1, 1, -1, 1, 1, -1,-1, 1,
    -1, 1, 1, -1, 1,-1, -1,-1,-1, -1,-1,-1, -1,-1, 1, -1, 1, 1,
     1, 1, 1,  1, 1,-1,  1,-1,-1,  1,-1,-1,  1,-1, 1,  1, 1, 1,
    -1,-1,-1,  1,-1,-1,  1,-1, 1,  1,-1, 1, -1,-1, 1, -1,-1,-1,
    -1, 1,-1,  1, 1,-1,  1, 1, 1,  1, 1, 1, -1, 1, 1, -1, 1,-1,
  };

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

  glBindVertexArray(0);
}

void Skybox::destroy() {
  if (cubemap) glDeleteTextures(1, &cubemap);
  if (vbo) glDeleteBuffers(1, &vbo);
  if (vao) glDeleteVertexArrays(1, &vao);
  cubemap = vao = vbo = 0;
}
