#include "Instancing.h"

void InstanceBuffer::create() {
  if (!buffer) glGenBuffers(1, &buffer);
}

void InstanceBuffer::destroy() {
  if (buffer) glDeleteBuffers(1, &buffer);
  buffer = 0;
  capacity = 0;
}

void InstanceBuffer::upload(const std::vector<glm::mat4>& models) {
  create();
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(models.size() * sizeof(glm::mat4)),
               models.data(), GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  capacity = (int)models.size();
}

void InstanceBuffer::bindToVAO(GLuint vao, int firstLocation) {
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);

  for (int i = 0; i < 4; ++i) {
    glEnableVertexAttribArray(firstLocation + i);
    glVertexAttribPointer(firstLocation + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                          (void*)(sizeof(float) * 4 * i));
    glVertexAttribDivisor(firstLocation + i, 1);
  }

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}
