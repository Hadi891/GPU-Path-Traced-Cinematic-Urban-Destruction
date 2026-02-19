#pragma once
#include <glad/glad.h>
#include <vector>

struct VertexPC {
  float px, py, pz;
  float cx, cy, cz;
};

class Mesh {
public:
  GLuint vao = 0, vbo = 0, ebo = 0;
  GLsizei indexCount = 0;

  void upload(const std::vector<VertexPC>& verts, const std::vector<unsigned>& indices);
  void destroy();

  void draw() const;
  void drawInstanced(int instanceCount) const;
};
