#pragma once
#include <string>
#include <vector>

unsigned int loadTextFile(const std::string& path, std::string& out);
void glCheckError(const char* label);
void* fileToBuffer(const std::string& path, size_t& outSize);
