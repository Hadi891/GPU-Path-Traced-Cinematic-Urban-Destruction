#include "GltfModel.h"

#include <iostream>
#include <unordered_map>
#include <cstring>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../dep/tinygltf/tiny_gltf.h"

static GLuint createGLTextureRGBA8(int w, int h, const unsigned char* rgba) {
  GLuint tex = 0;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);
  return tex;
}

static const unsigned char* accessorDataPtr(const tinygltf::Model& model,
                                            const tinygltf::Accessor& acc,
                                            int& outStrideBytes) {
  const tinygltf::BufferView& bv = model.bufferViews[acc.bufferView];
  const tinygltf::Buffer& buf = model.buffers[bv.buffer];

  int componentSize = tinygltf::GetComponentSizeInBytes(acc.componentType);
  int numComps = tinygltf::GetNumComponentsInType(acc.type);

  int stride = bv.byteStride ? (int)bv.byteStride : componentSize * numComps;
  outStrideBytes = stride;

  size_t offset = bv.byteOffset + acc.byteOffset;
  return buf.data.data() + offset;
}

static void uploadMeshPNUTriangles(const std::vector<float>& interleavedPNUV,
                                  const std::vector<unsigned int>& indices,
                                  GltfMeshGL& out) {
  const int strideFloats = 3 + 3 + 2;
  const int strideBytes = strideFloats * (int)sizeof(float);

  glGenVertexArrays(1, &out.vao);
  glGenBuffers(1, &out.vbo);
  glGenBuffers(1, &out.ebo);

  glBindVertexArray(out.vao);

  glBindBuffer(GL_ARRAY_BUFFER, out.vbo);
  glBufferData(GL_ARRAY_BUFFER,
               (GLsizeiptr)(interleavedPNUV.size() * sizeof(float)),
               interleavedPNUV.data(),
               GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out.ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               (GLsizeiptr)(indices.size() * sizeof(unsigned int)),
               indices.data(),
               GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, strideBytes, (void*)0);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, strideBytes, (void*)(3 * sizeof(float)));

  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, strideBytes, (void*)(6 * sizeof(float)));

  glBindVertexArray(0);

  out.indexCount = (int)indices.size();
}

void GltfMeshGL::destroy() {
  if (albedoTex) glDeleteTextures(1, &albedoTex);
  if (ebo) glDeleteBuffers(1, &ebo);
  if (vbo) glDeleteBuffers(1, &vbo);
  if (vao) glDeleteVertexArrays(1, &vao);
  albedoTex = ebo = vbo = vao = 0;
  indexCount = 0;
}

void GltfModelGL::destroy() {
  for (auto& m : meshes) m.destroy();
  meshes.clear();
}

static GLuint getBaseColorTexture(const tinygltf::Model& model, int materialIndex) {
  if (materialIndex < 0 || materialIndex >= (int)model.materials.size()) return 0;

  const tinygltf::Material& mat = model.materials[materialIndex];

  auto it = mat.values.find("baseColorTexture");
  if (it == mat.values.end()) return 0;

  int texIndex = it->second.TextureIndex();
  if (texIndex < 0 || texIndex >= (int)model.textures.size()) return 0;

  int imgIndex = model.textures[texIndex].source;
  if (imgIndex < 0 || imgIndex >= (int)model.images.size()) return 0;

  const tinygltf::Image& img = model.images[imgIndex];

  if (img.image.empty() || img.width <= 0 || img.height <= 0) return 0;

  std::vector<unsigned char> rgba;
  rgba.resize((size_t)img.width * (size_t)img.height * 4);

  if (img.component == 4) {
    std::memcpy(rgba.data(), img.image.data(), rgba.size());
  } else if (img.component == 3) {
    for (int i = 0; i < img.width * img.height; ++i) {
      rgba[i*4+0] = img.image[i*3+0];
      rgba[i*4+1] = img.image[i*3+1];
      rgba[i*4+2] = img.image[i*3+2];
      rgba[i*4+3] = 255;
    }
  } else {
    for (int i = 0; i < img.width * img.height; ++i) {
      rgba[i*4+0] = 180; rgba[i*4+1] = 180; rgba[i*4+2] = 180; rgba[i*4+3] = 255;
    }
  }

  return createGLTextureRGBA8(img.width, img.height, rgba.data());
}

bool loadGltfModel(const std::string& path, GltfModelGL& out) {
  out.destroy();

  tinygltf::TinyGLTF loader;
  tinygltf::Model model;
  std::string err, warn;

  bool ok = false;
  const bool isGLB = (path.size() >= 4 && path.substr(path.size()-4) == ".glb");
  if (isGLB) ok = loader.LoadBinaryFromFile(&model, &err, &warn, path);
  else       ok = loader.LoadASCIIFromFile(&model, &err, &warn, path);

  if (!warn.empty()) std::cerr << "[glTF warn] " << warn << "\n";
  if (!err.empty())  std::cerr << "[glTF err ] " << err << "\n";
  if (!ok) {
    std::cerr << "Failed to load glTF: " << path << "\n";
    return false;
  }

  int sceneIndex = model.defaultScene >= 0 ? model.defaultScene : 0;
  if (sceneIndex < 0 || sceneIndex >= (int)model.scenes.size()) {
    std::cerr << "No scene in glTF.\n";
    return false;
  }

  std::unordered_map<int, GLuint> baseColorTexCache;

  const tinygltf::Scene& scene = model.scenes[sceneIndex];

  auto processNode = [&](int nodeIndex, auto&& processNodeRef) -> void {
    const tinygltf::Node& node = model.nodes[nodeIndex];

    if (node.mesh >= 0) {
      const tinygltf::Mesh& mesh = model.meshes[node.mesh];

      for (const auto& prim : mesh.primitives) {
        if (prim.mode != TINYGLTF_MODE_TRIANGLES) continue;

        auto itPos = prim.attributes.find("POSITION");
        auto itNrm = prim.attributes.find("NORMAL");
        auto itUV  = prim.attributes.find("TEXCOORD_0");
        if (itPos == prim.attributes.end() || itNrm == prim.attributes.end() || itUV == prim.attributes.end()) {
          std::cerr << "Primitive missing POSITION/NORMAL/TEXCOORD_0\n";
          continue;
        }

        const tinygltf::Accessor& accPos = model.accessors[itPos->second];
        const tinygltf::Accessor& accNrm = model.accessors[itNrm->second];
        const tinygltf::Accessor& accUV  = model.accessors[itUV->second];

        if (accPos.count != accNrm.count || accPos.count != accUV.count) {
          std::cerr << "Accessor counts mismatch\n";
          continue;
        }

        if (prim.indices < 0) {
          std::cerr << "Primitive has no indices\n";
          continue;
        }
        const tinygltf::Accessor& accIdx = model.accessors[prim.indices];

        int stridePos=0, strideNrm=0, strideUV=0, strideIdx=0;
        const unsigned char* pPos = accessorDataPtr(model, accPos, stridePos);
        const unsigned char* pNrm = accessorDataPtr(model, accNrm, strideNrm);
        const unsigned char* pUV  = accessorDataPtr(model, accUV,  strideUV);
        const unsigned char* pIdx = accessorDataPtr(model, accIdx, strideIdx);

        std::vector<float> v;
        v.reserve((size_t)accPos.count * 8);

        for (size_t i = 0; i < accPos.count; ++i) {
          const float* pos = (const float*)(pPos + i * stridePos);
          const float* nrm = (const float*)(pNrm + i * strideNrm);
          const float* uv  = (const float*)(pUV  + i * strideUV);

          v.push_back(pos[0]); v.push_back(pos[1]); v.push_back(pos[2]);
          v.push_back(nrm[0]); v.push_back(nrm[1]); v.push_back(nrm[2]);
          v.push_back(uv[0]);  v.push_back(uv[1]);
        }

        std::vector<unsigned int> indices;
        indices.reserve((size_t)accIdx.count);

        for (size_t i = 0; i < accIdx.count; ++i) {
          unsigned int idx = 0;

          const unsigned char* ptr = pIdx + i * strideIdx;
          switch (accIdx.componentType) {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
              idx = *(const unsigned short*)ptr; break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
              idx = *(const unsigned int*)ptr; break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
              idx = *(const unsigned char*)ptr; break;
            default:
              std::cerr << "Unsupported index component type\n";
              idx = 0; break;
          }
          indices.push_back(idx);
        }

        GltfMeshGL outMesh;
        uploadMeshPNUTriangles(v, indices, outMesh);

        outMesh.cpuIdx = indices;
        outMesh.cpuPos.resize(accPos.count);
        outMesh.cpuNrm.resize(accPos.count);
        outMesh.cpuUV.resize(accPos.count);

        for (size_t i = 0; i < accPos.count; ++i) {
          outMesh.cpuPos[i] = glm::vec3(v[i*8+0], v[i*8+1], v[i*8+2]);
          outMesh.cpuNrm[i] = glm::vec3(v[i*8+3], v[i*8+4], v[i*8+5]);
          outMesh.cpuUV[i]  = glm::vec2(v[i*8+6], v[i*8+7]);
        }

        GLuint tex = 0;
        if (prim.material >= 0) {
          auto it = baseColorTexCache.find(prim.material);
          if (it != baseColorTexCache.end()) tex = it->second;
          else {
            tex = getBaseColorTexture(model, prim.material);
            baseColorTexCache[prim.material] = tex;
          }
        }
        outMesh.albedoTex = tex;

        out.meshes.push_back(outMesh);
      }
    }

    for (int child : node.children) processNodeRef(child, processNodeRef);
  };

  for (int nodeIndex : scene.nodes) processNode(nodeIndex, processNode);

  if (out.meshes.empty()) {
    std::cerr << "No drawable meshes were extracted from glTF.\n";
    return false;
  }

  std::cout << "Loaded glTF meshes: " << out.meshes.size() << "\n";
  return true;
}
