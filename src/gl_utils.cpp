#include "gl_utils.h"
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <iostream>

unsigned int loadTextFile(const std::string& path, std::string& out) {
  std::ifstream file(path, std::ios::in);
  if (!file) return 0;
  std::stringstream ss;
  ss << file.rdbuf();
  out = ss.str();
  return 1;
}

void glCheckError(const char* label) {
  GLenum err;
  bool any = false;
  while ((err = glGetError()) != GL_NO_ERROR) {
    any = true;
    std::cerr << "[GL ERROR] " << label << " : 0x" << std::hex << err << std::dec << "\n";
  }
  if (!any) {
  }
}

void* fileToBuffer(const std::string& path, size_t& outSize) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file) return nullptr;
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);
  char* buffer = new char[(size_t)size];
  if (!file.read(buffer, size)) {
    delete[] buffer;
    return nullptr;
  }
  outSize = (size_t)size;
  return buffer;
}
