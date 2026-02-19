#include "Raytrace.h"
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <numeric>
#include <cmath>

void RaytraceSceneGL::destroy() {
  if (ssboTris) glDeleteBuffers(1, &ssboTris);
  if (ssboNodes) glDeleteBuffers(1, &ssboNodes);
  ssboTris = ssboNodes = 0;
  triCount = nodeCount = 0;
}

static inline glm::vec3 vmin3(const glm::vec3& a, const glm::vec3& b){ return glm::min(a,b); }
static inline glm::vec3 vmax3(const glm::vec3& a, const glm::vec3& b){ return glm::max(a,b); }

struct TriRef {
  int triIdx;
  glm::vec3 c;
  glm::vec3 bmin;
  glm::vec3 bmax;
};

static void triBounds(const RTTriangle& t, glm::vec3& bmin, glm::vec3& bmax, glm::vec3& c){
  glm::vec3 p0 = glm::vec3(t.v0);
  glm::vec3 p1 = glm::vec3(t.v1);
  glm::vec3 p2 = glm::vec3(t.v2);
  bmin = vmin3(p0, vmin3(p1, p2));
  bmax = vmax3(p0, vmax3(p1, p2));
  c = (p0 + p1 + p2) * (1.0f/3.0f);
}

static int buildBVHRecursive(std::vector<BVHNode>& nodes,
                             std::vector<TriRef>& refs,
                             int start, int count)
{
  const int nodeIdx = (int)nodes.size();
  nodes.push_back({});

  glm::vec3 bmin( 1e30f), bmax(-1e30f);
  glm::vec3 cmin( 1e30f), cmax(-1e30f);

  for (int i = 0; i < count; i++) {
    const TriRef& r = refs[start + i];
    bmin = vmin3(bmin, r.bmin);
    bmax = vmax3(bmax, r.bmax);
    cmin = vmin3(cmin, r.c);
    cmax = vmax3(cmax, r.c);
  }

  nodes[nodeIdx].bmin = glm::vec4(bmin, 0.0f);
  nodes[nodeIdx].bmax = glm::vec4(bmax, 0.0f);

  const int LEAF_TRI_MAX = 8;
  if (count <= LEAF_TRI_MAX) {
    nodes[nodeIdx].leftChild  = -1;
    nodes[nodeIdx].rightChild = -1;
    nodes[nodeIdx].leftFirst  = start;
    nodes[nodeIdx].triCount   = count;
    return nodeIdx;
  }

  glm::vec3 ext = cmax - cmin;
  int axis = 0;
  if (ext.y > ext.x) axis = 1;
  if (ext.z > ext[axis]) axis = 2;

  const int mid = start + count / 2;
  std::nth_element(
    refs.begin() + start,
    refs.begin() + mid,
    refs.begin() + start + count,
    [axis](const TriRef& a, const TriRef& b) {
      return a.c[axis] < b.c[axis];
    }
  );

  nodes[nodeIdx].leftFirst = -1;
  nodes[nodeIdx].triCount  = 0;

  const int leftIdx  = buildBVHRecursive(nodes, refs, start, count / 2);
  const int rightIdx = buildBVHRecursive(nodes, refs, mid, count - count / 2);

  nodes[nodeIdx].leftChild  = leftIdx;
  nodes[nodeIdx].rightChild = rightIdx;

  return nodeIdx;
}


void buildAndUploadBVH(RaytraceSceneGL& rt, std::vector<RTTriangle>& tris)
{
  if (tris.empty()) {
    rt.triCount  = 0;
    rt.nodeCount = 0;

    if (!rt.ssboTris)  glGenBuffers(1, &rt.ssboTris);
    if (!rt.ssboNodes) glGenBuffers(1, &rt.ssboNodes);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, rt.ssboTris);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, rt.ssboNodes);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    return;
  }

  std::vector<TriRef> refs(tris.size());
  for (int i = 0; i < (int)tris.size(); i++){
    glm::vec3 bmin, bmax, c;
    triBounds(tris[i], bmin, bmax, c);
    refs[i] = { i, c, bmin, bmax };
  }

  std::vector<BVHNode> nodes;
  nodes.reserve(tris.size() * 2);

  buildBVHRecursive(nodes, refs, 0, (int)refs.size());

  std::vector<RTTriangle> trisOut(tris.size());
  for (int i = 0; i < (int)refs.size(); i++){
    trisOut[i] = tris[ refs[i].triIdx ];
  }
  tris = std::move(trisOut);

  rt.triCount = (int)tris.size();

  if (!rt.ssboTris)
    glGenBuffers(1, &rt.ssboTris);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, rt.ssboTris);
  glBufferData(GL_SHADER_STORAGE_BUFFER,
               rt.triCount * sizeof(RTTriangle),
               tris.data(),
               GL_DYNAMIC_DRAW);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  rt.nodeCount = (int)nodes.size();

  if (!rt.ssboNodes)
    glGenBuffers(1, &rt.ssboNodes);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, rt.ssboNodes);
  glBufferData(GL_SHADER_STORAGE_BUFFER,
               rt.nodeCount * sizeof(BVHNode),
               nodes.data(),
               GL_DYNAMIC_DRAW);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}
