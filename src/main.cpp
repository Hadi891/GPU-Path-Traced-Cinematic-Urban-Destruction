#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "Shader.h"
#include "gl_utils.h"
#include "Camera.h"
#include "Spline.h"
#include "Timeline.h"
#include "Mesh.h"
#include "Instancing.h"
#include "CityBuilder.h"
#include "Particles.h"
#include "PostProcess.h"
#include "ShadowMap.h"
#include "gltf/GltfModel.h"
#include "Skybox.h"
#include "Texture2D.h"
#include "Raytrace.h"
#include "PhysicsWorld.h"
#include "GeometryDemo.h"
#include "stb_image_write.h"
#define STB_EASY_FONT_IMPLEMENTATION
#include "../dep/stb_easy_font.h"
#include <unordered_map>
#include <functional>

static void forceRepeatWrap(GLuint tex) {
  if (!tex) return;
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glBindTexture(GL_TEXTURE_2D, 0);
}

static void saveAO_PNG(GLuint aoTex, int w, int h, const char* filename) {
  std::vector<float> pixels(w*h*4);
  glBindTexture(GL_TEXTURE_2D, aoTex);
  glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, pixels.data());
  glBindTexture(GL_TEXTURE_2D, 0);

  std::vector<unsigned char> out(w*h*3);
  for (int i=0;i<w*h;i++){
    float ao = pixels[i*4+0];
    ao = glm::clamp(ao, 0.0f, 1.0f);
    unsigned char v = (unsigned char)(ao * 255.0f);
    out[i*3+0] = v;
    out[i*3+1] = v;
    out[i*3+2] = v;
  }
  stbi_write_png(filename, w, h, 3, out.data(), w*3);
}

static void saveColor_PNG(GLuint tex, int w, int h, const char* filename) {
  std::vector<float> pixels(w*h*4);
  glBindTexture(GL_TEXTURE_2D, tex);
  glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, pixels.data());
  glBindTexture(GL_TEXTURE_2D, 0);

  std::vector<unsigned char> out(w*h*4);

  for (int y = 0; y < h; ++y) {
    int srcY = y;
    int dstY = (h - 1 - y);

    for (int x = 0; x < w; ++x) {
      int src = (srcY*w + x) * 4;
      int dst = (dstY*w + x) * 4;

      float r = glm::clamp(pixels[src+0], 0.0f, 1.0f);
      float g = glm::clamp(pixels[src+1], 0.0f, 1.0f);
      float b = glm::clamp(pixels[src+2], 0.0f, 1.0f);
      float a = glm::clamp(pixels[src+3], 0.0f, 1.0f);

      out[dst+0] = (unsigned char)(r * 255.0f);
      out[dst+1] = (unsigned char)(g * 255.0f);
      out[dst+2] = (unsigned char)(b * 255.0f);
      out[dst+3] = (unsigned char)(a * 255.0f);
    }
  }

  stbi_write_png(filename, w, h, 4, out.data(), w*4);
}

static void glfwErrorCallback(int code, const char* msg) {
  std::cerr << "[GLFW ERROR] " << code << " : " << msg << "\n";
}

static void framebufferSizeCallback(GLFWwindow*, int w, int h) {
  glViewport(0, 0, w, h);
}

static std::vector<int> gChunkMeshIndex;
static bool g_firstMouse = true;
static double g_lastX = 0.0, g_lastY = 0.0;
static bool g_mouseLook = false;
static GLuint gTexArray = 0;
static int gTexArrayW = 0, gTexArrayH = 0, gTexArrayLayers = 0;
static bool gCollapseStarted = false;
static bool gPhysicsReady = false;
static std::vector<glm::vec3> gChunkLocalCenter;
static float gCollapseScale = 0.01f;
static glm::mat4 gCollapseBase = glm::mat4(1.0f);
static glm::mat4 gCollapseStaticM = glm::mat4(1.0f);
static float gCollapseTimer = 0.0f;
static const float gStaticHold = 0.25f;
static float gPhysicsTimeScale = 4.0f;
static float gMaxPhysDt = 1.0f / 60.0f;
static float gDustBurstTimer = 0.0f;
static const float gDustBurstDuration = 10.f;
const int DUST_COUNT = 40000;

static bool  gQuakeActive = false;
static float gQuakeT = 0.0f;
static float gQuakeDuration = 7.f;
static float gQuakeAmpPos = 0.35f;
static float gQuakeAmpRot = 3.5f;
static float gQuakeAmpFov = 7.0f;
static float gQuakeFreq = 32.0f;

static glm::vec3 gCamBasePos(0.0f);
static float gCamBaseYaw = 0.0f;
static float gCamBasePitch = 0.0f;

static bool  rtRecording = false;
static int   rtFrame = 0;
static const int RT_FPS = 20;
static bool  rtCollapseSeen = false;
static float rtAfterTimer = 0.0f;

static bool   gGeoDragging = false;
static double gGeoLastX = 0.0, gGeoLastY = 0.0;
static float  gGeoYawDeg   = 0.0f;
static float  gGeoPitchDeg = -10.0f;
static float  gGeoDist     = 10.0f;
static glm::vec3 gGeoTarget(0.0f);
static float gBaseVolGeo = 0.0f;

static GeometryDemo gGeoLap;
static GeometryDemo gGeoTaubin;
static GeometryDemo gGeoCurv;
static bool gGeoCacheReady = false;
static float gVolLap = 0.f;
static float gVolTaubin = 0.f;
static float gVolCurv = 0.f;

static bool  gCollapseTriggered = false;
static float gEarthquakeDelay = 2.0f;
static float gCollapseDelay   = 4.0f;
static float gEventTimer = 0.0f;
static float gCollapseDelayTimer = 0.0f;

static bool  gWalkCam = true;
static float gWalkU = 0.0f;
static float gWalkSpeed = 1.0f;
static float gPathLen = 1.0f;
static float gWalkStopZ = -31.0f;
static bool  gWalkStopped = false;

static float gScanT = 0.0f;
static float gScanMaxYaw = 18.0f;
static float gScanFreq = 0.45f;
static float gMicroJitter = 2.0f;

static float gStepFreq = 1.85f;
static float gBobY = 0.06f;
static float gBobX = 0.03f;
static float gRollDeg = 1.3f;

static float gFocusBlend = 0.0f;
static float gFocusBlendSpeed = 1.8f;

static float gGlanceTimer = 0.0f;
static float gGlanceDuration = 0.0f;
static float gNextGlanceDelay = 0.0f;
static float gGlanceYawTarget = 0.0f;
static bool  gIsGlancing = false;

static float hash11(float x) {
  return glm::fract(sinf(x * 127.1f) * 43758.5453f);
}

static float randRange(float a, float b, float seed) {
  return a + (b - a) * hash11(seed);
}

static float smooth01(float x) { return x * x * (3.0f - 2.0f * x); }

static float noise1(float t) {
  float i = floorf(t);
  float f = t - i;
  float a = hash11(i);
  float b = hash11(i + 1.0f);
  float u = smooth01(f);
  float v = a + (b - a) * u;
  return v * 2.0f - 1.0f;
}

static void ensureTexArray(GLuint& texArray,
                           int& outW, int& outH, int& outLayers,
                           const std::vector<GLuint>& texList)
{
  int layers = (int)texList.size();
  if (layers <= 0) return;

  int targetW = 0, targetH = 0;
  GLint internalFmt = GL_RGBA8;

  for (int i = 0; i < layers; ++i) {
    glBindTexture(GL_TEXTURE_2D, texList[i]);
    int w=0, h=0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);

    if (i == 0) {
      glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFmt);
    }

    targetW = std::max(targetW, w);
    targetH = std::max(targetH, h);
  }
  glBindTexture(GL_TEXTURE_2D, 0);

  bool needRebuild = (texArray == 0) || (targetW != outW) || (targetH != outH) || (layers != outLayers);
  if (!needRebuild) return;

  if (texArray == 0) glGenTextures(1, &texArray);
  outW = targetW;
  outH = targetH;
  outLayers = layers;

  glBindTexture(GL_TEXTURE_2D_ARRAY, texArray);
  glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalFmt, targetW, targetH, layers,
               0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

  static GLuint scratchTex = 0;
  static GLuint fboRead = 0, fboDraw = 0;

  if (!scratchTex) glGenTextures(1, &scratchTex);
  glBindTexture(GL_TEXTURE_2D, scratchTex);
  glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, targetW, targetH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);

  if (!fboRead) glGenFramebuffers(1, &fboRead);
  if (!fboDraw) glGenFramebuffers(1, &fboDraw);

  for (int i = 0; i < layers; ++i) {
    GLuint src = texList[i];

    glBindTexture(GL_TEXTURE_2D, src);
    int srcW=0, srcH=0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &srcW);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &srcH);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, fboRead);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, src, 0);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboDraw);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, scratchTex, 0);

    glBlitFramebuffer(
      0, 0, srcW, srcH,
      0, 0, targetW, targetH,
      GL_COLOR_BUFFER_BIT, GL_LINEAR
    );

    glCopyImageSubData(
      scratchTex, GL_TEXTURE_2D,      0, 0, 0, 0,
      texArray,   GL_TEXTURE_2D_ARRAY,0, 0, 0, i,
      targetW, targetH, 1
    );
  }

  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

static void mouseCallback(GLFWwindow* win, double x, double y) {
  if (!g_mouseLook) return;
  if (g_firstMouse) { g_lastX = x; g_lastY = y; g_firstMouse = false; }

  double dx = x - g_lastX;
  double dy = g_lastY - y;
  g_lastX = x; g_lastY = y;

  Camera* cam = (Camera*)glfwGetWindowUserPointer(win);
  if (!cam) return;

  const float sens = 0.08f;
  cam->yawDeg   += (float)dx * sens;
  cam->pitchDeg += (float)dy * sens;
  cam->pitchDeg = glm::clamp(cam->pitchDeg, -89.f, 89.f);
}


static void drawGltfInstancedTextured(
  Shader& textured,
  const GltfModelGL& model,
  InstanceBuffer& inst,
  int instanceCount,
  const glm::mat4& viewProj,
  const glm::mat4& lightViewProj,
  const glm::mat4& preTransform,
  const glm::vec3& sunDir,
  const glm::vec3& sunColor,
  const glm::vec3& skyColor,
  float ambient,
  Camera cam,
  GLuint shadowTex,
  int shadowSize
) {
  textured.use();
  textured.setMat4("uViewProj", &viewProj[0][0]);
  textured.setMat4("uLightViewProj", &lightViewProj[0][0]);

  textured.setMat4("uPre", &preTransform[0][0]);

  textured.setVec3("uSunDir", sunDir.x, sunDir.y, sunDir.z);
  textured.setVec3("uSunColor", sunColor.x, sunColor.y, sunColor.z);
  textured.setVec3("uSkyColor", skyColor.x, skyColor.y, skyColor.z);
  textured.setFloat("uAmbient", ambient);
  textured.setVec3("uCamPos", cam.pos.x, cam.pos.y, cam.pos.z);

  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, shadowTex);
  glUniform1i(glGetUniformLocation(textured.id(), "uShadowMap"), 2);
  glUniform2f(glGetUniformLocation(textured.id(), "uShadowMapSize"),
              (float)shadowSize, (float)shadowSize);

  for (auto& m : model.meshes) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m.albedoTex ? m.albedoTex : 0);
    glUniform1i(glGetUniformLocation(textured.id(), "uAlbedo"), 0);

    inst.bindToVAO(m.vao, 3);

    glBindVertexArray(m.vao);
    glDrawElementsInstanced(GL_TRIANGLES, m.indexCount, GL_UNSIGNED_INT, 0, instanceCount);
  }
  glBindVertexArray(0);
}

static void drawGltfInstancedDepth(
  Shader& shadowShader,
  const GltfModelGL& model,
  InstanceBuffer& inst,
  int instanceCount,
  const glm::mat4& lightViewProj,
  const glm::mat4& preTransform
) {
  shadowShader.use();
  shadowShader.setMat4("uLightViewProj", &lightViewProj[0][0]);
  shadowShader.setMat4("uPre", &preTransform[0][0]);

  for (auto& m : model.meshes) {
    inst.bindToVAO(m.vao, 3);
    glBindVertexArray(m.vao);
    glDrawElementsInstanced(GL_TRIANGLES, m.indexCount, GL_UNSIGNED_INT, 0, instanceCount);
  }
  glBindVertexArray(0);
}

static void appendModelTrianglesWorld(
  std::vector<RTTriangle>& out,
  const GltfModelGL& model,
  const glm::mat4& worldFromModel,
  const std::function<int(GLuint)>& getTexIndex
){
  for (const auto& m : model.meshes) {
    int texId = getTexIndex(m.albedoTex);
    if (m.cpuIdx.empty()) continue;

    for (size_t i = 0; i + 2 < m.cpuIdx.size(); i += 3) {
      unsigned ia = m.cpuIdx[i+0];
      unsigned ib = m.cpuIdx[i+1];
      unsigned ic = m.cpuIdx[i+2];

      glm::vec3 p0 = glm::vec3(worldFromModel * glm::vec4(m.cpuPos[ia], 1.0));
      glm::vec3 p1 = glm::vec3(worldFromModel * glm::vec4(m.cpuPos[ib], 1.0));
      glm::vec3 p2 = glm::vec3(worldFromModel * glm::vec4(m.cpuPos[ic], 1.0));

      glm::vec3 n0 = glm::normalize(glm::mat3(worldFromModel) * m.cpuNrm[ia]);
      glm::vec3 n1 = glm::normalize(glm::mat3(worldFromModel) * m.cpuNrm[ib]);
      glm::vec3 n2 = glm::normalize(glm::mat3(worldFromModel) * m.cpuNrm[ic]);

      RTTriangle t;
      t.v0 = glm::vec4(p0, 0);
      t.v1 = glm::vec4(p1, 0);
      t.v2 = glm::vec4(p2, 0);

      t.n0 = glm::vec4(n0, 0);
      t.n1 = glm::vec4(n1, 0);
      t.n2 = glm::vec4(n2, 0);


      glm::vec2 uvA(0.f), uvB(0.f), uvC(0.f);

      bool hasUV = !m.cpuUV.empty();

      if (hasUV) {
        uvA = m.cpuUV[ia];
        uvB = m.cpuUV[ib];
        uvC = m.cpuUV[ic];
      }

      t.uv0 = glm::vec4(uvA, 0, 0);
      t.uv1 = glm::vec4(uvB, 0, 0);
      t.uv2 = glm::vec4(uvC, 0, 0);

      t.meta = glm::ivec4(texId, 0, 0, 0);

      out.push_back(t);
    }
  }
}

static void flattenGltfToIndexed(
  const GltfModelGL& model,
  std::vector<glm::vec3>& outPos,
  std::vector<glm::uvec3>& outTri
){
  outPos.clear();
  outTri.clear();

  uint32_t base = 0;
  for (const auto& m : model.meshes) {
    if (m.cpuPos.empty() || m.cpuIdx.empty()) continue;

    outPos.insert(outPos.end(), m.cpuPos.begin(), m.cpuPos.end());

    for (size_t i=0; i+2<m.cpuIdx.size(); i+=3) {
      uint32_t a = base + (uint32_t)m.cpuIdx[i+0];
      uint32_t b = base + (uint32_t)m.cpuIdx[i+1];
      uint32_t c = base + (uint32_t)m.cpuIdx[i+2];
      outTri.push_back(glm::uvec3(a,b,c));
    }

    base += (uint32_t)m.cpuPos.size();
  }
}

static void geoMouseButtonCallback(GLFWwindow* win, int button, int action, int mods) {
  (void)mods;
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      gGeoDragging = true;
      glfwGetCursorPos(win, &gGeoLastX, &gGeoLastY);
    } else if (action == GLFW_RELEASE) {
      gGeoDragging = false;
    }
  }
}

static void geoCursorPosCallback(GLFWwindow* win, double x, double y) {
  if (!gGeoDragging) return;

  double dx = x - gGeoLastX;
  double dy = y - gGeoLastY;
  gGeoLastX = x;
  gGeoLastY = y;

  const float sens = 0.25f;
  gGeoYawDeg   += (float)dx * sens;
  gGeoPitchDeg += (float)dy * sens;

  gGeoPitchDeg = glm::clamp(gGeoPitchDeg, -89.0f, 89.0f);
}

static void geoScrollCallback(GLFWwindow* win, double xoff, double yoff) {
  (void)win; (void)xoff;
  gGeoDist *= (yoff > 0.0 ? 0.9f : 1.1f);
  gGeoDist = glm::clamp(gGeoDist, 2.0f, 50.0f);
}

static GLuint gTextProg = 0;
static GLuint gTextVAO  = 0;
static GLuint gTextVBO  = 0;

static GLuint compileShader(GLenum type, const char* src) {
  GLuint s = glCreateShader(type);
  glShaderSource(s, 1, &src, nullptr);
  glCompileShader(s);
  GLint ok = 0;
  glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[2048];
    glGetShaderInfoLog(s, 2048, nullptr, log);
    std::cerr << "[TEXT] shader compile error:\n" << log << "\n";
  }
  return s;
}

static GLuint createTextProgram() {
  const char* vs = R"(
    #version 430 core
    layout(location=0) in vec2 aPosPx;
    uniform vec2 uScreen;
    uniform vec2 uOrigin;
    uniform float uScale;

    void main() {
      vec2 p = uOrigin + (aPosPx - uOrigin) * uScale;

      float x = (p.x / uScreen.x) * 2.0 - 1.0;
      float y = 1.0 - (p.y / uScreen.y) * 2.0;
      gl_Position = vec4(x, y, 0.0, 1.0);
    }
  )";

  const char* fs = R"(
    #version 430 core
    out vec4 FragColor;
    uniform vec3 uColor;
    void main() {
      FragColor = vec4(uColor, 1.0);
    }
  )";

  GLuint V = compileShader(GL_VERTEX_SHADER, vs);
  GLuint F = compileShader(GL_FRAGMENT_SHADER, fs);

  GLuint p = glCreateProgram();
  glAttachShader(p, V);
  glAttachShader(p, F);
  glLinkProgram(p);

  glDeleteShader(V);
  glDeleteShader(F);

  GLint ok = 0;
  glGetProgramiv(p, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[2048];
    glGetProgramInfoLog(p, 2048, nullptr, log);
    std::cerr << "[TEXT] program link error:\n" << log << "\n";
  }
  return p;
}

static void initTextOverlay() {
  if (gTextProg) return;

  gTextProg = createTextProgram();

  glGenVertexArrays(1, &gTextVAO);
  glGenBuffers(1, &gTextVBO);

  glBindVertexArray(gTextVAO);
  glBindBuffer(GL_ARRAY_BUFFER, gTextVBO);
  glBufferData(GL_ARRAY_BUFFER, 1024 * 1024, nullptr, GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

static void drawText2D(int screenW, int screenH, int x, int y, const char* str,
                       float r=1.f, float g=1.f, float b=1.f)
{
  if (!gTextProg) return;

  static float buf[99999 / 4];

  int quadCount = stb_easy_font_print((float)x, (float)y, (char*)str, nullptr,
                                      (char*)buf, sizeof(buf));
  if (quadCount <= 0) return;

  std::vector<float> tri;
  tri.reserve(quadCount * 6 * 2);

  for (int i = 0; i < quadCount; ++i) {
    float* v = buf + i * 16;

    float x0 = v[0],  y0 = v[1];
    float x1 = v[4],  y1 = v[5];
    float x2 = v[8],  y2 = v[9];
    float x3 = v[12], y3 = v[13];

    tri.insert(tri.end(), { x0,y0, x1,y1, x2,y2,  x0,y0, x2,y2, x3,y3 });
  }

  GLboolean wasDepth = glIsEnabled(GL_DEPTH_TEST);
  GLboolean wasCull  = glIsEnabled(GL_CULL_FACE);
  GLboolean wasSciss = glIsEnabled(GL_SCISSOR_TEST);
  GLboolean wasBlend = glIsEnabled(GL_BLEND);

  glUseProgram(gTextProg);
  float scale = 2.5f;
  glUniform1f(glGetUniformLocation(gTextProg, "uScale"), scale);
  glUniform2f(glGetUniformLocation(gTextProg, "uOrigin"), (float)x, (float)y);
  glUniform2f(glGetUniformLocation(gTextProg, "uScreen"), (float)screenW, (float)screenH);
  glUniform3f(glGetUniformLocation(gTextProg, "uColor"), r, g, b);

  glBindVertexArray(gTextVAO);
  glBindBuffer(GL_ARRAY_BUFFER, gTextVBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, tri.size() * sizeof(float), tri.data());

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glDisable(GL_SCISSOR_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(tri.size() / 2));

  if (!wasBlend) glDisable(GL_BLEND);
  if (wasSciss) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
  if (wasCull)  glEnable(GL_CULL_FACE);    else glDisable(GL_CULL_FACE);
  if (wasDepth) glEnable(GL_DEPTH_TEST);   else glDisable(GL_DEPTH_TEST);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}


static float computeAABBVolume(const std::vector<glm::vec3>& pos) {
  if (pos.empty()) return 0.f;
  glm::vec3 mn( 1e30f), mx(-1e30f);
  for (auto& p : pos) {
    mn = glm::min(mn, p);
    mx = glm::max(mx, p);
  }
  glm::vec3 d = mx - mn;
  return std::max(0.f, d.x) * std::max(0.f, d.y) * std::max(0.f, d.z);
}

static void appendOneMeshTrianglesWorld(
  std::vector<RTTriangle>& out,
  const GltfModelGL& model,
  int meshIndex,
  const glm::mat4& worldFromModel,
  const std::function<int(GLuint)>& getTexIndex
){
  if (meshIndex < 0 || meshIndex >= (int)model.meshes.size()) return;
  const auto& m = model.meshes[meshIndex];

  int texId = getTexIndex(m.albedoTex);
  if (m.cpuIdx.empty()) return;

  for (size_t i = 0; i + 2 < m.cpuIdx.size(); i += 3) {
    unsigned ia = m.cpuIdx[i+0];
    unsigned ib = m.cpuIdx[i+1];
    unsigned ic = m.cpuIdx[i+2];

    glm::vec3 p0 = glm::vec3(worldFromModel * glm::vec4(m.cpuPos[ia], 1.0));
    glm::vec3 p1 = glm::vec3(worldFromModel * glm::vec4(m.cpuPos[ib], 1.0));
    glm::vec3 p2 = glm::vec3(worldFromModel * glm::vec4(m.cpuPos[ic], 1.0));

    glm::vec3 n0 = glm::normalize(glm::mat3(worldFromModel) * m.cpuNrm[ia]);
    glm::vec3 n1 = glm::normalize(glm::mat3(worldFromModel) * m.cpuNrm[ib]);
    glm::vec3 n2 = glm::normalize(glm::mat3(worldFromModel) * m.cpuNrm[ic]);

    RTTriangle t;
    t.v0 = glm::vec4(p0, 0);
    t.v1 = glm::vec4(p1, 0);
    t.v2 = glm::vec4(p2, 0);

    t.n0 = glm::vec4(n0, 0);
    t.n1 = glm::vec4(n1, 0);
    t.n2 = glm::vec4(n2, 0);

    glm::vec2 uvA(0.f), uvB(0.f), uvC(0.f);
    bool hasUV = !m.cpuUV.empty();
    if (hasUV) {
      uvA = m.cpuUV[ia];
      uvB = m.cpuUV[ib];
      uvC = m.cpuUV[ic];
    }

    t.uv0 = glm::vec4(uvA, 0, 0);
    t.uv1 = glm::vec4(uvB, 0, 0);
    t.uv2 = glm::vec4(uvC, 0, 0);

    t.meta = glm::ivec4(texId, 0, 0, 0);

    out.push_back(t);
  }
}

static void appendCollapseChunksRT(
    std::vector<RTTriangle>& tris,
    const GltfModelGL& collapseModel,
    PhysicsWorld& physics,
    const std::function<int(GLuint)>& getTexIndex
) {
  int chunkCount = std::min((int)physics.chunks.size(), (int)gChunkMeshIndex.size());

  for (int i = 0; i < chunkCount; i++) {
    int mi = gChunkMeshIndex[i];
    auto& m = collapseModel.meshes[mi];
    if (m.cpuIdx.empty()) continue;

    btTransform t;
    physics.chunks[i].body->getMotionState()->getWorldTransform(t);
    btVector3 p = t.getOrigin();
    btQuaternion q = t.getRotation();

    glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(p.x(), p.y(), p.z()));
    glm::mat4 R = glm::mat4_cast(glm::quat(q.w(), q.x(), q.y(), q.z()));
    glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(gCollapseScale));
    glm::mat4 C = glm::translate(glm::mat4(1.0f), -gChunkLocalCenter[i]);

    glm::mat4 worldFromModel = T * R * S * C;

    appendOneMeshTrianglesWorld(tris, collapseModel, mi, worldFromModel, getTexIndex);
  }
}


int main() {
  glfwSetErrorCallback(glfwErrorCallback);
  if (!glfwInit()) return -1;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow* win = glfwCreateWindow(1280, 720, "Main Screen", nullptr, nullptr);
  if (!win) { glfwTerminate(); return -1; }
  glfwMakeContextCurrent(win);
  glfwSwapInterval(1);
  glfwSetFramebufferSizeCallback(win, framebufferSizeCallback);

  Camera cam;
  glfwSetWindowUserPointer(win, &cam);
  glfwSetCursorPosCallback(win, mouseCallback);


  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Failed to init GLAD\n";
    return -1;
  }

  std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  glEnable(GL_FRAMEBUFFER_SRGB);

  std::vector<VertexPC> cubeVerts = {
    {-0.5f,-0.5f,-0.5f,  0.35f,0.35f,0.38f},
    { 0.5f,-0.5f,-0.5f,  0.35f,0.35f,0.38f},
    { 0.5f, 0.5f,-0.5f,  0.35f,0.35f,0.38f},
    {-0.5f, 0.5f,-0.5f,  0.35f,0.35f,0.38f},

    {-0.5f,-0.5f, 0.5f,  0.35f,0.35f,0.38f},
    { 0.5f,-0.5f, 0.5f,  0.35f,0.35f,0.38f},
    { 0.5f, 0.5f, 0.5f,  0.35f,0.35f,0.38f},
    {-0.5f, 0.5f, 0.5f,  0.35f,0.35f,0.38f},
  };

  std::vector<unsigned> cubeIdx = {
    0,1,2, 2,3,0,
    4,5,6, 6,7,4,
    0,3,7, 7,4,0,
    1,5,6, 6,2,1,
    0,1,5, 5,4,0,
    3,2,6, 6,7,3
  };

  Mesh cube;
  cube.upload(cubeVerts, cubeIdx);


  CityBuilder city;
  city.P.seed = 1337;
  city.P.buildingCountPerSide = 6;
  city.P.spacingZ = 7.0f;
  city.P.streetHalfWidth = 9.0f;
  city.P.baseHeight = 6.0f;
  city.P.heightVar = 0.f;
  city.generate();

  std::vector<std::vector<glm::mat4>> groups(6);

  for (size_t i = 0; i < city.buildings.size(); ++i) {
    int g = (int)(i % 6);
    groups[g].push_back(city.buildings[i]);
  }

  std::vector<InstanceBuffer> inst(6);
  for (int g = 0; g < 6; ++g) {
    inst[g].upload(groups[g]);
  }

  std::vector<glm::mat4> streetModels;

  float streetZ0     = 5.0f;
  float streetZEnd   = -39.0f;
  float streetStepZ  = 22.0f;
  float streetY      = -0.55f;

  for (float z = streetZ0; z >= streetZEnd; z -= streetStepZ) {
    glm::mat4 M(1.0f);
    M = glm::translate(M, glm::vec3(0.0f, streetY, z));
    streetModels.push_back(M);
  }

  InstanceBuffer streetInst;
  streetInst.upload(streetModels);

  std::vector<glm::mat4> groundModelsGLB;
  {
    glm::mat4 M(1.0f);
    M = glm::translate(M, glm::vec3(0.0f, -0.55f, -30.0f));
    groundModelsGLB.push_back(M);
  }

  InstanceBuffer groundInst;
  groundInst.upload(groundModelsGLB);

  Shader basic;
  if (!basic.loadGraphics("shaders/basic.vert", "shaders/basic.frag")) return -1;

  Shader particlesCompute;
  if (!particlesCompute.loadCompute("shaders/particles.comp")) return -1;

  Shader particlesRender;
  if (!particlesRender.loadGraphics("shaders/particles_sand.vert", "shaders/particles_sand.frag")) return -1;

  GLuint sandTex = loadTextureRGBA("assets/particles/sand.png", true);
  if (!sandTex) return -1;
  GLuint dustTex = loadTextureRGBA("assets/particles/dust2.png", true);
  if (!dustTex) return -1;

  Shader skyboxShader;
  if (!skyboxShader.loadGraphics("shaders/skybox.vert", "shaders/skybox.frag")) return -1;

  Skybox sky;
  sky.createCube();

  std::array<std::string, 6> faces = {
    "assets/skybox_apoc/px.png",
    "assets/skybox_apoc/nx.png",
    "assets/skybox_apoc/py.png",
    "assets/skybox_apoc/ny.png",
    "assets/skybox_apoc/pz.png",
    "assets/skybox_apoc/nz.png",
  };
  if (!sky.loadCubemap(faces, true)) return -1;

  Particles particles;
  particles.init(100000);

  Shader fog;
  if (!fog.loadGraphics("shaders/fullscreen.vert", "shaders/fog_godrays.frag")) return -1;

  Shader textured;
  if (!textured.loadGraphics("shaders/textured.vert", "shaders/textured.frag")) return -1;

  std::vector<GltfModelGL> buildings(6);
  for (int i = 0; i < 6; ++i) {
    std::string path = "assets/building_0" + std::to_string(i) + ".glb";
    if (!loadGltfModel(path, buildings[i])) return -1;
  }

  GltfModelGL streetModel;
  if (!loadGltfModel("assets/street_00.glb", streetModel)) return -1;

  GltfModelGL groundModel;
  if (!loadGltfModel("assets/ground_00.glb", groundModel)) return -1;

  GltfModelGL collapseModel;
  if (!loadGltfModel("assets/collapse_building3.glb", collapseModel)) return -1;

  Shader geomDemoShader;
  if (!geomDemoShader.loadGraphics("shaders/geom_demo.vert", "shaders/geom_demo.frag")) return -1;

  GeometryDemo geomDemoBase;
  GeometryDemo geomDemo;
  {
    std::vector<glm::vec3> demoPos;
    std::vector<glm::uvec3> demoTri;
    flattenGltfToIndexed(collapseModel, demoPos, demoTri);

    geomDemoBase.buildFromIndexed(demoPos, demoTri);
    geomDemoBase.setSolidColor(glm::vec3(0.85f));
    geomDemoBase.uploadToGPU();

    geomDemo.buildFromIndexed(demoPos, demoTri);
    geomDemo.setSolidColor(glm::vec3(0.85f));
    geomDemo.uploadToGPU();
  }

  {
    std::vector<glm::vec3> demoPos;
    std::vector<glm::uvec3> demoTri;
    flattenGltfToIndexed(collapseModel, demoPos, demoTri);
    geomDemoBase.buildFromIndexed(demoPos, demoTri);
    geomDemoBase.uploadToGPU();
  }

  geomDemo.uploadToGPU();

  bool geoMode = false;
  int  geoView = 2;
  bool geoDirty = true;
  GLFWwindow* geoWin = nullptr;

  GltfModelGL collapseModelGeo;
  GeometryDemo geomDemoBaseGeo;
  GeometryDemo geomDemoGeo;
  bool geoGpuReady = false;

  geomDemo.iters = 60;
  geomDemo.lambda = 0.55f;
  geomDemo.mu = -0.53f;

  InstanceBuffer collapseStaticInst;
  {
      std::vector<glm::mat4> I(1, glm::mat4(1.0f));
      collapseStaticInst.upload(I);
  }

  gCollapseStaticM = glm::mat4(1.0f);

  gCollapseStaticM = glm::translate(
      gCollapseStaticM,
      glm::vec3(0.1f, 0.2f, -55.0f)
  );

  gCollapseStaticM = glm::rotate(
      gCollapseStaticM,
      glm::radians(-90.f),
      glm::vec3(0, 1, 0)
  );

  gCollapseStaticM = glm::scale(
      gCollapseStaticM,
      glm::vec3(0.01f)
  );

  std::cout << "[COLLAPSE] meshes loaded = " << collapseModel.meshes.size() << "\n";

  size_t totalIdx = 0, totalPos = 0;
  int nonEmpty = 0;

  for (size_t i = 0; i < collapseModel.meshes.size(); ++i) {
    auto& m = collapseModel.meshes[i];
    totalIdx += m.cpuIdx.size();
    totalPos += m.cpuPos.size();
    if (!m.cpuIdx.empty()) nonEmpty++;

    if (i < 10) {
      std::cout << "  mesh[" << i << "] pos=" << m.cpuPos.size()
                << " idx=" << m.cpuIdx.size()
                << " indexCount=" << m.indexCount << "\n";
    }
  }

  std::cout << "[COLLAPSE] nonEmptyMeshes=" << nonEmpty
            << " totalPos=" << totalPos
            << " totalIdx=" << totalIdx << "\n";

  for (auto& mesh : collapseModel.meshes) forceRepeatWrap(mesh.albedoTex);


  gCollapseBase = glm::mat4(1.0f);
  gCollapseBase = glm::translate(gCollapseBase, glm::vec3(0.1f, 0.2f, -55.0f));

  glm::quat collapseRot = glm::angleAxis(glm::radians(-90.f), glm::vec3(0,1,0));

  PhysicsWorld physics;
  physics.addGround();

  gChunkLocalCenter.clear();
  gChunkMeshIndex.clear();

  gChunkLocalCenter.reserve(collapseModel.meshes.size());
  gChunkMeshIndex.reserve(collapseModel.meshes.size());

  for (int mi = 0; mi < (int)collapseModel.meshes.size(); ++mi) {
    auto& m = collapseModel.meshes[mi];
    if (m.cpuPos.empty() || m.cpuIdx.empty()) continue;

    glm::vec3 mn( 1e9f), mx(-1e9f);
    for (auto& p : m.cpuPos) {
      mn = glm::min(mn, p);
      mx = glm::max(mx, p);
    }
    glm::vec3 center = 0.5f * (mn + mx);
    glm::vec3 half   = 0.5f * (mx - mn);

    gChunkMeshIndex.push_back(mi);
    gChunkLocalCenter.push_back(center);

    glm::vec3 cScaled = center * gCollapseScale;
    glm::vec3 cRot    = collapseRot * cScaled;
    glm::vec3 baseT   = glm::vec3(gCollapseBase[3]);
    glm::vec3 worldCenter = baseT + cRot;

    glm::vec3 halfScaled = half * gCollapseScale;
    halfScaled *= 0.65f;
    halfScaled = glm::max(halfScaled, glm::vec3(0.008f));

    auto chunk = physics.addChunk(worldCenter, halfScaled, 5.0f);

    btTransform start;
    start.setIdentity();
    start.setOrigin(btVector3(worldCenter.x, worldCenter.y, worldCenter.z));
    start.setRotation(btQuaternion(collapseRot.x, collapseRot.y, collapseRot.z, collapseRot.w));
    chunk.body->setWorldTransform(start);
    chunk.body->getMotionState()->setWorldTransform(start);

    chunk.body->setDamping(0.25f, 0.55f);
    chunk.body->setFriction(0.9f);
    chunk.body->setRestitution(0.05f);

    chunk.body->setActivationState(ISLAND_SLEEPING);
  }

  gPhysicsReady = true;

  for (auto& M : buildings) {
    for (auto& mesh : M.meshes) forceRepeatWrap(mesh.albedoTex);
  }
  for (auto& mesh : streetModel.meshes) forceRepeatWrap(mesh.albedoTex);
  for (auto& mesh : groundModel.meshes) forceRepeatWrap(mesh.albedoTex);

  std::unordered_map<GLuint, int> texToIdx;
  std::vector<GLuint> texList;

  auto getTexIndex = [&](GLuint tex) -> int {
    if (!tex) return -1;
    auto it = texToIdx.find(tex);
    if (it != texToIdx.end()) return it->second;
    int idx = (int)texList.size();
    texList.push_back(tex);
    texToIdx[tex] = idx;
    return idx;
  };

  RaytraceSceneGL rt;
  std::vector<RTTriangle> tris;


  glm::mat4 preStreetRT(1.0f);
  preStreetRT = glm::translate(preStreetRT, glm::vec3(0.f, 1.5f, 0.f));
  preStreetRT = glm::rotate(preStreetRT, glm::radians(0.f), glm::vec3(1,0,0));
  preStreetRT = glm::scale(preStreetRT, glm::vec3(0.6f));

  glm::mat4 preGroundRT(1.0f);
  preGroundRT = glm::translate(preGroundRT, glm::vec3(0.f, -6.f, 50.f));
  preGroundRT = glm::rotate(preGroundRT, glm::radians(90.f), glm::vec3(0,1,0));
  preGroundRT = glm::scale(preGroundRT, glm::vec3(1.9f));

  float buildingYOffset[6] = {-0.45f,-0.45f,-0.45f,-0.45f,-0.45f,-0.45f};

  glm::mat4 preBuildingRT[6];
  for (int g=0; g<6; ++g) {
    glm::mat4 pre(1.0f);
    pre = glm::translate(pre, glm::vec3(0.1f, buildingYOffset[g], 0.f));
    pre = glm::rotate(pre, glm::radians(90.f), glm::vec3(1,0,0));
    pre = glm::scale(pre, glm::vec3(0.25f));
    preBuildingRT[g] = pre;
  }

  tris.clear();

  for (int g=0; g<6; ++g) {
    for (const glm::mat4& instM : groups[g]) {
      glm::mat4 worldFromModel = instM * preBuildingRT[g];

      appendModelTrianglesWorld(tris, buildings[g], worldFromModel, getTexIndex);
    }
  }

  for (const glm::mat4& instM : streetModels) {
    glm::mat4 worldFromModel = instM * preStreetRT;

    appendModelTrianglesWorld(tris, streetModel, worldFromModel, getTexIndex);
  }

  for (const glm::mat4& instM : groundModelsGLB) {
    glm::mat4 worldFromModel = instM * preGroundRT;

    appendModelTrianglesWorld(tris, groundModel, worldFromModel, getTexIndex);

  }

  if (!gCollapseStarted) {
    glm::mat4 worldFromModel = gCollapseStaticM;
    appendModelTrianglesWorld(tris, collapseModel, worldFromModel, getTexIndex);
  }


  rt.triCount = (int)tris.size();
  std::cout << "[RT] Triangles: " << rt.triCount << "\n";

  buildAndUploadBVH(rt, tris);
  std::cout << "[BVH] Nodes: " << rt.nodeCount << "\n";

  Offscreen off;
  FullscreenQuad fsq;
  fsq.create();

  Shader shadowShader;
  if (!shadowShader.loadGraphics("shaders/shadow_depth.vert", "shaders/shadow_depth.frag")) return -1;

  Shader rtPT;
  if (!rtPT.loadCompute("shaders/rt_pt.comp")) return -1;



  Shader resolve;
  if (!resolve.loadGraphics("shaders/fullscreen.vert", "shaders/resolve_accum.frag")) return -1;

  Shader compAO;
  if (!compAO.loadGraphics("shaders/fullscreen.vert", "shaders/composite_ao.frag")) return -1;

  ShadowMap shadowMap;
  shadowMap.create(2048);


  Timeline timeline;
  bool cineMode = false;
  float cineDuration = 115.f;

  CatmullRomSpline camPath;
  camPath.loop = false;
  camPath.points = {
    glm::vec3(0.f, 1.6f,  8.f),
    glm::vec3(0.f, 1.6f,  4.f),
    glm::vec3(0.f, 1.6f, -10.f),
    glm::vec3(0.2f, 1.8f, -25.f),
    glm::vec3(-0.2f, 1.6f, -40.f),
    glm::vec3(0.f, 1.6f, -60.f),
  };

  {
    const int N = 200;
    float len = 0.0f;
    glm::vec3 prev = camPath.sample(0.0f);
    for (int i = 1; i <= N; ++i) {
      float u = (float)i / (float)N;
      glm::vec3 p = camPath.sample(u);
      len += glm::length(p - prev);
      prev = p;
    }
    gPathLen = std::max(0.001f, len);
    std::cout << "[WALKCAM] path length ~= " << gPathLen << " m\n";
  }

  GLuint aoTex = 0;
  int aoW = 0, aoH = 0;

  GLuint finalTex = 0;
  int finalW = 0, finalH = 0;

  GLuint accumTex = 0;
  int accumW=0, accumH=0;

  auto ensureAccum = [&](int w, int h){
    if (accumTex && accumW==w && accumH==h) return;
    if (!accumTex) glGenTextures(1, &accumTex);
    accumW=w; accumH=h;

    glBindTexture(GL_TEXTURE_2D, accumTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
  };

  auto ensureFinal = [&](int w, int h){
    if (finalTex && finalW==w && finalH==h) return;
    if (!finalTex) glGenTextures(1, &finalTex);
    finalW=w; finalH=h;

    glBindTexture(GL_TEXTURE_2D, finalTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
  };

  auto ensureAO = [&](int w, int h){
    if (aoTex && aoW==w && aoH==h) return;

    if (!aoTex) glGenTextures(1, &aoTex);
    aoW=w; aoH=h;

    glBindTexture(GL_TEXTURE_2D, aoTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);

    const float one[4] = {1.f, 1.f, 1.f, 1.f};
    glClearTexImage(aoTex, 0, GL_RGBA, GL_FLOAT, one);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
  };


  while (!glfwWindowShouldClose(win)) {
    
    static double lastTime = glfwGetTime();
    double now = glfwGetTime();
    float dtReal = (float)(now - lastTime);
    lastTime = now;

    float dt = dtReal;

    if (rtRecording) dt = 1.0f / float(RT_FPS);

    if (gCollapseStarted) {
      gCollapseTimer += dt;
      float dtPhys = std::min(dt, gMaxPhysDt) * gPhysicsTimeScale;
      physics.step(dtPhys);
    }

    if (gDustBurstTimer > 0.0f) gDustBurstTimer -= dt;
    bool dustBurstActive = (gDustBurstTimer > 0.0f);

    static bool prevR = false;
    bool nowR = (glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS);
    bool pressedR = nowR && !prevR;
    prevR = nowR;
    if (pressedR && !rtRecording) {
      rtRecording = true;
      rtCollapseSeen = false;
      rtAfterTimer = 0.0f;
      rtFrame = 0;
      std::cout << "[RT] Recording started\n";
    }

    if (glfwGetKey(win, GLFW_KEY_C) == GLFW_PRESS) cineMode = true;
    if (glfwGetKey(win, GLFW_KEY_V) == GLFW_PRESS) cineMode = false;

    if (glfwGetKey(win, GLFW_KEY_M) == GLFW_PRESS) {
      g_mouseLook = true;
      glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      g_firstMouse = true;
    }
    if (glfwGetKey(win, GLFW_KEY_N) == GLFW_PRESS) {
      g_mouseLook = false;
      glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      g_firstMouse = true;
    }

    auto pressed = [&](int key){
      static std::unordered_map<int,bool> prev;
      bool nowK = glfwGetKey(win, key) == GLFW_PRESS;
      bool was  = prev[key];
      prev[key] = nowK;
      return nowK && !was;
    };

    if (pressed(GLFW_KEY_G)) {
      geoMode = !geoMode;

      if (geoMode) {
        if (!geoWin) {
          glfwWindowHint(GLFW_DEPTH_BITS, 24);
          glfwWindowHint(GLFW_STENCIL_BITS, 8);

          geoWin = glfwCreateWindow(1280, 940, "Geometry Demo - Collapse Building", nullptr, win);
          glfwSetMouseButtonCallback(geoWin, geoMouseButtonCallback);
          glfwSetCursorPosCallback(geoWin, geoCursorPosCallback);
          glfwSetScrollCallback(geoWin, geoScrollCallback);
          if (!geoWin) {
            std::cerr << "Failed to create geo window\n";
            geoMode = false;
          } else {
            glfwSetWindowPos(geoWin, 60, 60);

            glfwMakeContextCurrent(geoWin);
            glfwSwapInterval(1);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glFrontFace(GL_CCW);
            glEnable(GL_FRAMEBUFFER_SRGB);

            initTextOverlay();
            if (!loadGltfModel("assets/collapse_building.glb", collapseModelGeo)) {
              std::cerr << "Failed to load collapse model for geo window\n";
            } else {
              for (auto& mesh : collapseModelGeo.meshes) forceRepeatWrap(mesh.albedoTex);

              std::vector<glm::vec3> demoPos;
              std::vector<glm::uvec3> demoTri;
              flattenGltfToIndexed(collapseModelGeo, demoPos, demoTri);

              geomDemoBaseGeo.buildFromIndexed(demoPos, demoTri);
              geomDemoBaseGeo.uploadToGPU();

              gBaseVolGeo = computeAABBVolume(geomDemoBaseGeo.pos);

              gGeoLap   = geomDemoBaseGeo;
              gGeoTaubin= geomDemoBaseGeo;
              gGeoCurv  = geomDemoBaseGeo;

              gGeoLap.applyLaplacian(geomDemo.iters, geomDemo.lambda);
              gGeoLap.setSolidColor(glm::vec3(0.2f, 1.0f, 0.2f));
              gGeoLap.uploadToGPU();
              gVolLap = computeAABBVolume(gGeoLap.pos);

              {
                float mu = -(geomDemo.lambda + 0.02f);
                gGeoTaubin.applyTaubin(geomDemo.iters, geomDemo.lambda, mu);
                gGeoTaubin.setSolidColor(glm::vec3(1.0f, 0.7f, 0.2f));
                gGeoTaubin.uploadToGPU();
                gVolTaubin = computeAABBVolume(gGeoTaubin.pos);
              }

              gGeoCurv.computeCurvatureColors();
              gGeoCurv.uploadToGPU();
              gVolCurv = computeAABBVolume(gGeoCurv.pos);

              gGeoCacheReady = true;
              geoGpuReady = true;
            }

            glfwMakeContextCurrent(win);
          }
        }
      } else {
        if (geoWin) {
          glfwDestroyWindow(geoWin);
          geoWin = nullptr;
          geoGpuReady = false;
        }
      }
    }
    if (pressed(GLFW_KEY_1)) { geomDemo.mode = 1; geoDirty = true; }
    if (pressed(GLFW_KEY_2)) { geomDemo.mode = 2; geoDirty = true; }
    if (pressed(GLFW_KEY_3)) { geomDemo.mode = 3; geoDirty = true; }
    if (pressed(GLFW_KEY_4)) { geomDemo.mode = 4; geoDirty = true; }

    if (pressed(GLFW_KEY_LEFT_BRACKET))  { geomDemo.iters = std::max(0, geomDemo.iters - 1); geoDirty = true; }
    if (pressed(GLFW_KEY_RIGHT_BRACKET)) { geomDemo.iters = std::min(200, geomDemo.iters + 1); geoDirty = true; }

    if (pressed(GLFW_KEY_MINUS))         { geomDemo.lambda = std::max(0.0f, geomDemo.lambda - 0.05f); geoDirty = true; }
    if (pressed(GLFW_KEY_EQUAL))         { geomDemo.lambda = std::min(1.0f, geomDemo.lambda + 0.05f); geoDirty = true; }

    if (pressed(GLFW_KEY_I)) { geomDemo.resetToBase(); geoDirty = true; }

    timeline.update(dt);
    if (rtRecording && rtCollapseSeen) {
      rtAfterTimer += dt;
    }

    if (geoMode && geoDirty) {
      geomDemo = geomDemoBase;

      if (geomDemo.mode == 1) {
        geomDemo.setSolidColor(glm::vec3(0.85f));
      }
      else if (geomDemo.mode == 2) {
        geomDemo.applyLaplacian(geomDemo.iters, geomDemo.lambda);
        geomDemo.setSolidColor(glm::vec3(0.2f, 1.0f, 0.2f));
      }
      else if (geomDemo.mode == 3) {
        float mu = -(geomDemo.lambda + 0.02f);
        geomDemo.applyTaubin(geomDemo.iters, geomDemo.lambda, mu);
        geomDemo.setSolidColor(glm::vec3(1.0f, 0.7f, 0.2f));
      }
      else if (geomDemo.mode == 4) {
        geomDemo.computeCurvatureColors();
      }

      geomDemo.uploadToGPU();
      geoDirty = false;
    }

if (rtRecording) {
      if (!gWalkStopped) {
        gWalkU += (gWalkSpeed * dt) / gPathLen;
        gWalkU = glm::clamp(gWalkU, 0.0f, 1.0f);
      }

      glm::vec3 p = camPath.sample(gWalkU);
      glm::vec3 tng = camPath.tangent(gWalkU);
      if (glm::length(tng) < 1e-6f) tng = glm::vec3(0,0,-1);
      tng = glm::normalize(tng);

      if (!gWalkStopped && p.z <= gWalkStopZ) {
        gWalkStopped = true;
        gFocusBlend = 0.0f;
      }

      float timeNow = (float)now;
      float step = timeNow * (glm::two_pi<float>() * gStepFreq);

      float bobY = sinf(step) * gBobY;
      float bobX = sinf(step * 0.5f + 1.3f) * gBobX;

      float rubbleX = noise1(timeNow * 2.2f) * 0.015f;
      float rubbleY = noise1(timeNow * 2.8f + 5.0f) * 0.012f;

      glm::vec3 right = glm::normalize(glm::cross(tng, glm::vec3(0,1,0)));
      if (glm::length(right) < 1e-6f) right = glm::vec3(1,0,0);

      cam.pos = p;

      if (!gWalkStopped) {
        cam.pos += right * (bobX + rubbleX);
        cam.pos += glm::vec3(0, bobY + rubbleY, 0);
      } else {
        cam.pos.z = gWalkStopZ;
      }

      gGlanceTimer += dt;

      if (gIsGlancing) {
        if (gGlanceTimer >= gGlanceDuration) {
          gIsGlancing = false;
          gGlanceTimer = 0.0f;

          gNextGlanceDelay = randRange(1.5f, 4.0f, timeNow * 1.37f);
        }
      }
      else {
        if (gGlanceTimer >= gNextGlanceDelay) {
          gIsGlancing = true;
          gGlanceTimer = 0.0f;

          gGlanceDuration = randRange(1.2f, 2.2f, timeNow * 2.11f);

          float side = (hash11(timeNow * 3.77f) > 0.5f) ? 1.0f : -1.0f;

          float strength = randRange(30.0f, 55.0f, timeNow * 4.91f);

          gGlanceYawTarget = side * strength;
        }
      }

      float yawOff = 0.0f;
      if (gIsGlancing) {
        float t = gGlanceTimer / gGlanceDuration;
        float smooth = sinf(t * glm::pi<float>());
        yawOff = gGlanceYawTarget * smooth;
      }

      float micro = noise1(timeNow * 6.0f) * 0.8f;
      float pitchOff = noise1(timeNow * 4.0f + 11.0f) * 1.0f;
      yawOff += micro;

      float baseYaw = glm::degrees(atan2(tng.z, tng.x));
      float basePitch = glm::degrees(asin(glm::clamp(tng.y, -1.f, 1.f)));

      float desiredYaw = baseYaw + yawOff;
      float desiredPitch = glm::clamp(basePitch + pitchOff, -30.f, 30.f);

      if (gWalkStopped) {
        glm::vec3 target(0.1f, 0.8f, -55.0f);

        glm::vec3 dir = glm::normalize(target - cam.pos);
        float focusYaw = glm::degrees(atan2(dir.z, dir.x));
        float focusPitch = glm::degrees(asin(glm::clamp(dir.y, -1.f, 1.f)));

        gFocusBlend = glm::clamp(gFocusBlend + dt * gFocusBlendSpeed, 0.0f, 1.0f);

        desiredYaw   = glm::mix(desiredYaw, focusYaw, gFocusBlend);
        desiredPitch = glm::mix(desiredPitch, focusPitch, gFocusBlend);
      }

      auto smoothAngle = [&](float cur, float target, float s){
        float d = target - cur;
        while (d > 180.f) d -= 360.f;
        while (d < -180.f) d += 360.f;
        return cur + d * glm::clamp(s, 0.f, 1.f);
      };

      float rotSmooth = 1.0f - expf(-dt * 4.0f);
      cam.yawDeg   = smoothAngle(cam.yawDeg, desiredYaw, rotSmooth);
      cam.pitchDeg = glm::clamp(
        smoothAngle(cam.pitchDeg, desiredPitch, rotSmooth),
        -89.f, 89.f
      );
} else if (!cineMode) {
      float speed = (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ? 8.0f : 3.0f;
      glm::vec3 f = cam.forward();
      glm::vec3 r = cam.right();

      if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) cam.pos += f * speed * dt;
      if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) cam.pos -= f * speed * dt;
      if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) cam.pos += r * speed * dt;
      if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) cam.pos -= r * speed * dt;
      if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS) cam.pos.y += speed * dt;
      if (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS) cam.pos.y -= speed * dt;
    } else {
      float t01 = glm::clamp(timeline.t / cineDuration, 0.f, 1.f);
      glm::vec3 p = camPath.sample(t01);
      glm::vec3 dir = camPath.tangent(t01);

      cam.pos = p;
      cam.yawDeg = glm::degrees(std::atan2(dir.z, dir.x));
      cam.pitchDeg = glm::degrees(std::asin(glm::clamp(dir.y, -1.f, 1.f)));
    }



    if (gQuakeActive) {
      gQuakeT += dt;
      float t = gQuakeT;

      float k = 1.0f - glm::clamp(t / gQuakeDuration, 0.0f, 1.0f);
      float env = k * k;

      gCamBasePos = cam.pos;
      gCamBaseYaw = cam.yawDeg;
      gCamBasePitch = cam.pitchDeg;

      float n1 = hash11(t * gQuakeFreq + 1.0f) * 2.0f - 1.0f;
      float n2 = hash11(t * gQuakeFreq + 2.0f) * 2.0f - 1.0f;
      float n3 = hash11(t * gQuakeFreq + 3.0f) * 2.0f - 1.0f;

      glm::vec3 right = cam.right();
      glm::vec3 up    = glm::vec3(0,1,0);
      glm::vec3 fwd   = cam.forward();

      cam.pos = gCamBasePos
              + right * (n1 * gQuakeAmpPos * env)
              + up    * (n2 * gQuakeAmpPos * 0.6f * env)
              + fwd   * (n3 * gQuakeAmpPos * 0.3f * env);


      cam.yawDeg   = gCamBaseYaw   + (n2 * gQuakeAmpRot * env);
      cam.pitchDeg = glm::clamp(gCamBasePitch + (n1 * gQuakeAmpRot * 0.6f * env), -89.f, 89.f);

      if (t >= gQuakeDuration) {
        gQuakeActive = false;
        cam.pos = gCamBasePos;
        cam.yawDeg = gCamBaseYaw;
        cam.pitchDeg = gCamBasePitch;
      }
    }


    if (gPhysicsReady && !gCollapseTriggered && cam.pos.z < -30.0f) {
        gCollapseTriggered = true;
        gEventTimer = 0.0f;
    }

    if (gCollapseTriggered && !gCollapseStarted) {
        gEventTimer += dt;

        if (!gQuakeActive && gEventTimer >= gEarthquakeDelay) {
            gQuakeActive = true;
            gQuakeT = 0.0f;
        }

        if (gEventTimer >= gCollapseDelay) {
            gCollapseStarted = true;
            gCollapseTimer = 0.0f;
            if (rtRecording && !rtCollapseSeen) {
                rtCollapseSeen = true;
                rtAfterTimer = 0.0f;
                std::cout << "[RT] Collapse detected\n";
            }

            for (auto& c : physics.chunks) {
                c.body->setActivationState(ACTIVE_TAG);
                c.body->activate(true);
                c.body->setLinearVelocity(btVector3(0,0,0));
                c.body->setAngularVelocity(btVector3(0,0,0));
            }

            gDustBurstTimer = gDustBurstDuration;
        }
    }

    bool drawStaticCollapse = (!gCollapseStarted) || (gCollapseTimer < gStaticHold);
    bool drawChunksCollapse = (gCollapseStarted) && (gCollapseTimer >= gStaticHold);

    glfwPollEvents();
    if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(win, 1);

    int w, h;
    glfwGetFramebufferSize(win, &w, &h);
    float aspect = (h == 0) ? 1.0f : (float)w / (float)h;
    off.create(w, h);

    glm::vec3 sunDir   = glm::normalize(glm::vec3(-0.35f, -0.75f, -0.55f));
    glm::vec3 sunColor(1.0f, 0.75f, 0.55f);
    glm::vec3 skyColor(0.25f, 0.35f, 0.55f);
    float ambient = 0.25f;

    glm::vec3 sunDirForShading = -sunDir;

    glm::vec3 focus   = cam.pos + cam.forward() * 35.0f;
    glm::vec3 lightPos = focus - sunDir * 160.0f;

    glm::mat4 lightView = glm::lookAt(lightPos, focus, glm::vec3(0,1,0));

    float orthoSize = 55.0f;
    float nearL = 5.0f, farL = 180.0f;
    glm::mat4 lightProj = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearL, farL);

    glm::mat4 lightViewProj = lightProj * lightView;

    bool pauseMain = (geoMode && geoWin);
    if (!pauseMain){

      off.bind();
      glViewport(0, 0, w, h);
      glDisable(GL_SCISSOR_TEST);
      glScissor(0, 0, w, h);
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      glClearColor(0.02f, 0.02f, 0.03f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glm::mat4 proj = cam.proj(aspect);
      glm::mat4 view = cam.view();

      glDepthMask(GL_FALSE);
      glDepthFunc(GL_LEQUAL);
      glDisable(GL_CULL_FACE);

      skyboxShader.use();
      skyboxShader.setMat4("uView", &view[0][0]);
      skyboxShader.setMat4("uProj", &proj[0][0]);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_CUBE_MAP, sky.cubemap);
      glUniform1i(glGetUniformLocation(skyboxShader.id(), "uSky"), 0);

      glBindVertexArray(sky.vao);
      glDrawArrays(GL_TRIANGLES, 0, 36);
      glBindVertexArray(0);

      glEnable(GL_CULL_FACE);
      glDepthFunc(GL_LESS);
      glDepthMask(GL_TRUE);

      glm::vec3 windDir = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));
      float windAmp = 0.8f;

      glm::vec3 emitCenter(0.f, 0.2f, -25.f);
      glm::vec3 emitHalf(12.f, 10.f, 35.f);

      glm::vec3 dustCenter = glm::vec3(0.1f, 0.2f, -55.0f);
      glm::vec3 dustHalf = glm::vec3(12.f, 8.f, 10.f);

      particles.dispatchCompute(
        particlesCompute.id(),
        dt,
        timeline.t,
        emitCenter,
        emitHalf,
        windDir,
        windAmp,
        dustBurstActive,
        dustCenter,
        dustHalf,
        DUST_COUNT
      );
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

      float azimuth   = glm::radians(220.0);
      float elevation = glm::radians(12.0);
      glm::vec3 sunDir = glm::normalize(glm::vec3(-0.35f, -0.75f, -0.55f));
      glm::vec3 sunColor(1.0f, 0.75f, 0.55f);
      glm::vec3 skyColor(0.25f, 0.35f, 0.55f);
      float ambient = 0.25f;

      glm::vec3 focus = cam.pos + cam.forward() * 35.0f;
      glm::vec3 lightPos = focus - sunDir * 160.0f;

      glm::mat4 lightView = glm::lookAt(lightPos, focus, glm::vec3(0,1,0));

      float orthoSize = 55.0f;
      float nearL = 5.0f, farL = 180.0f;
      glm::mat4 lightProj = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearL, farL);

      glm::mat4 lightViewProj = lightProj * lightView;

      shadowMap.begin();

      glEnable(GL_POLYGON_OFFSET_FILL);
      glPolygonOffset(2.0f, 4.0f);
      glDisable(GL_CULL_FACE);

      shadowShader.use();
      shadowShader.setMat4("uLightViewProj", &lightViewProj[0][0]);

      float buildingScale[6] = {0.08f, 0.08f, 0.08f, 0.08f, 0.08f, 0.08f};
      float buildingYOffset[6] = {-0.45f, -0.45f, -0.45f, -0.45f, -0.45f, -0.45f};

      glm::mat4 preStreet(1.0f);

      preStreet = glm::translate(preStreet, glm::vec3(0.f, 1.5f, 0.f));
      preStreet = glm::rotate(preStreet, glm::radians(0.f), glm::vec3(1,0,0));
      preStreet = glm::scale(preStreet, glm::vec3(0.6f));

      glm::mat4 preGround(1.0f);
      preGround = glm::translate(preGround, glm::vec3(0.f, -6.f, 50.f));
      preGround = glm::rotate(preGround, glm::radians(90.f), glm::vec3(0,1,0));
      preGround = glm::scale(preGround, glm::vec3(1.9f));

      for (int g = 0; g < 6; ++g) {
        int count = (int)groups[g].size();
        if (count == 0) continue;

        glm::mat4 pre(1.0f);
        pre = glm::translate(pre, glm::vec3(0.1f, buildingYOffset[g], 0.f));
        pre = glm::rotate(pre, glm::radians(90.f), glm::vec3(1,0,0));
        pre = glm::scale(pre, glm::vec3(0.25f));

        drawGltfInstancedDepth(
          shadowShader,
          buildings[g],
          inst[g],
          count,
          lightViewProj,
          pre
        );
      }

      drawGltfInstancedDepth(
        shadowShader,
        streetModel,
        streetInst,
        (int)streetModels.size(),
        lightViewProj,
        preStreet
      );

      drawGltfInstancedDepth(
        shadowShader,
        groundModel,
        groundInst,
        1,
        lightViewProj,
        preGround
      );


      if (gPhysicsReady) {
        if (drawStaticCollapse) {
          shadowShader.use();
          shadowShader.setMat4("uLightViewProj", &lightViewProj[0][0]);
          shadowShader.setMat4("uPre", &gCollapseStaticM[0][0]);

          for (auto& mesh : collapseModel.meshes) {
            if (mesh.indexCount == 0) continue;
            glBindVertexArray(mesh.vao);
            glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
          }
          glBindVertexArray(0);
        } else if (drawChunksCollapse) {
          int chunkCount = std::min((int)physics.chunks.size(), (int)gChunkMeshIndex.size());

          for (int i = 0; i < chunkCount; i++) {
            int mi = gChunkMeshIndex[i];
            auto& mesh = collapseModel.meshes[mi];
            auto& body = physics.chunks[i].body;
            if (mesh.indexCount == 0) continue;

            btTransform t;
            body->getMotionState()->getWorldTransform(t);
            btVector3 p = t.getOrigin();
            btQuaternion q = t.getRotation();

            glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(p.x(), p.y(), p.z()));
            glm::mat4 R = glm::mat4_cast(glm::quat(q.w(), q.x(), q.y(), q.z()));
            glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(gCollapseScale));
            glm::mat4 C = glm::translate(glm::mat4(1.0f), -gChunkLocalCenter[i]);
            glm::mat4 M = T * R * S * C;

            shadowShader.use();
            shadowShader.setMat4("uLightViewProj", &lightViewProj[0][0]);
            shadowShader.setMat4("uPre", &M[0][0]);

            glBindVertexArray(mesh.vao);
            glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
          }
          glBindVertexArray(0);
        }
      }

      glDisable(GL_POLYGON_OFFSET_FILL);

      shadowMap.end(w, h);

      off.bind();
      glViewport(0, 0, w, h);

      glEnable(GL_CULL_FACE);


      glm::mat4 model = glm::mat4(1.0f);
      glm::mat4 mvp = proj * view * model;
      glm::mat4 viewProj = proj * view;
      
      basic.use();
      basic.setMat4("uViewProj", &viewProj[0][0]);

      basic.setMat4("uLightViewProj", &lightViewProj[0][0]);

      glActiveTexture(GL_TEXTURE2);
      glBindTexture(GL_TEXTURE_2D, shadowMap.depthTex);
      glUniform1i(glGetUniformLocation(basic.id(), "uShadowMap"), 2);
      glUniform2f(glGetUniformLocation(basic.id(), "uShadowMapSize"),
                  (float)shadowMap.size, (float)shadowMap.size);



      glm::vec3 sunDirForShading = -sunDir;
      basic.setVec3("uSunDir", sunDirForShading.x, sunDirForShading.y, sunDirForShading.z);
      basic.setVec3("uSunColor", sunColor.x, sunColor.y, sunColor.z);
      basic.setVec3("uSkyColor", skyColor.x, skyColor.y, skyColor.z);
      basic.setFloat("uAmbient", ambient);

      for (int g = 0; g < 6; ++g) {
        int count = (int)groups[g].size();
        if (count == 0) continue;

        glm::mat4 pre(1.0f);
        pre = glm::translate(pre, glm::vec3(0.1f, buildingYOffset[g], 0.f));
        pre = glm::rotate(pre, glm::radians(90.f), glm::vec3(1,0,0));
        pre = glm::scale(pre, glm::vec3(0.25f));

        drawGltfInstancedTextured(
          textured,
          buildings[g],
          inst[g],
          count,
          viewProj,
          lightViewProj,
          pre,
          sunDirForShading,
          sunColor,
          skyColor,
          ambient,
          cam,
          shadowMap.depthTex, shadowMap.size
        );
      }

      textured.use();

      textured.setMat4("uViewProj", &viewProj[0][0]);
      textured.setMat4("uLightViewProj", &lightViewProj[0][0]);
      textured.setMat4("uPre", &gCollapseStaticM[0][0]);

      textured.setVec3("uSunDir",
          sunDirForShading.x,
          sunDirForShading.y,
          sunDirForShading.z
      );
      textured.setVec3("uSunColor", sunColor.x, sunColor.y, sunColor.z);
      textured.setVec3("uSkyColor", skyColor.x, skyColor.y, skyColor.z);
      textured.setFloat("uAmbient", ambient);
      textured.setVec3("uCamPos", cam.pos.x, cam.pos.y, cam.pos.z);

      glActiveTexture(GL_TEXTURE2);
      glBindTexture(GL_TEXTURE_2D, shadowMap.depthTex);
      glUniform1i(glGetUniformLocation(textured.id(), "uShadowMap"), 2);
      glUniform2f(
          glGetUniformLocation(textured.id(), "uShadowMapSize"),
          (float)shadowMap.size,
          (float)shadowMap.size
      );


      if (drawStaticCollapse) {
        textured.setMat4("uPre", &gCollapseStaticM[0][0]);

        for (auto& mesh : collapseModel.meshes) {
          if (mesh.indexCount == 0) continue;
          glActiveTexture(GL_TEXTURE0);
          glBindTexture(GL_TEXTURE_2D, mesh.albedoTex ? mesh.albedoTex : 0);
          glUniform1i(glGetUniformLocation(textured.id(), "uAlbedo"), 0);

          collapseStaticInst.bindToVAO(mesh.vao, 3);
          glBindVertexArray(mesh.vao);
          glDrawElementsInstanced(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0, 1);
        }
        glBindVertexArray(0);
      }

      drawGltfInstancedTextured(
        textured,
        streetModel,
        streetInst,
        (int)streetModels.size(),
        viewProj,
        lightViewProj,
        preStreet,
        sunDirForShading,
        sunColor,
        skyColor,
        ambient,
        cam,
        shadowMap.depthTex, shadowMap.size
      );

      drawGltfInstancedTextured(
        textured,
        groundModel,
        groundInst,
        1,
        viewProj,
        lightViewProj,
        preGround,
        sunDirForShading,
        sunColor,
        skyColor,
        ambient,
        cam,
        shadowMap.depthTex, shadowMap.size
      );

      if (drawChunksCollapse && gPhysicsReady) {
        int chunkCount = std::min((int)physics.chunks.size(), (int)gChunkMeshIndex.size());

        textured.use();
        textured.setMat4("uViewProj", &viewProj[0][0]);
        textured.setMat4("uLightViewProj", &lightViewProj[0][0]);
        textured.setVec3("uSunDir", sunDirForShading.x, sunDirForShading.y, sunDirForShading.z);
        textured.setVec3("uSunColor", sunColor.x, sunColor.y, sunColor.z);
        textured.setVec3("uSkyColor", skyColor.x, skyColor.y, skyColor.z);
        textured.setFloat("uAmbient", ambient);
        textured.setVec3("uCamPos", cam.pos.x, cam.pos.y, cam.pos.z);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, shadowMap.depthTex);
        glUniform1i(glGetUniformLocation(textured.id(), "uShadowMap"), 2);
        glUniform2f(glGetUniformLocation(textured.id(), "uShadowMapSize"),
                    (float)shadowMap.size, (float)shadowMap.size);

        for (int i = 0; i < chunkCount; i++) {
          int mi = gChunkMeshIndex[i];
          auto& mesh = collapseModel.meshes[mi];
          auto& body = physics.chunks[i].body;

          btTransform t;
          body->getMotionState()->getWorldTransform(t);
          btVector3 p = t.getOrigin();
          btQuaternion q = t.getRotation();

          glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(p.x(), p.y(), p.z()));
          glm::mat4 R = glm::mat4_cast(glm::quat(q.w(), q.x(), q.y(), q.z()));
          glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(gCollapseScale));
          glm::mat4 C = glm::translate(glm::mat4(1.0f), -gChunkLocalCenter[i]);

          glm::mat4 M = T * R * S * C;

          textured.setMat4("uPre", &M[0][0]);

          glActiveTexture(GL_TEXTURE0);
          glBindTexture(GL_TEXTURE_2D, mesh.albedoTex ? mesh.albedoTex : 0);
          glUniform1i(glGetUniformLocation(textured.id(), "uAlbedo"), 0);

          glBindVertexArray(mesh.vao);
          glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);
      }

      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      glEnable(GL_PROGRAM_POINT_SIZE);

      glDepthMask(GL_FALSE);

      glUseProgram(particlesRender.id());

      glUniformMatrix4fv(glGetUniformLocation(particlesRender.id(), "uView"), 1, GL_FALSE, &view[0][0]);
      glUniformMatrix4fv(glGetUniformLocation(particlesRender.id(), "uProj"), 1, GL_FALSE, &proj[0][0]);

      glUniform3f(glGetUniformLocation(particlesRender.id(), "uSandTint"), 0.95f, 0.85f, 0.65f);
      glUniform3f(glGetUniformLocation(particlesRender.id(), "uDustTint"), 0.7f, 0.7f, 0.7f);

      float baseSize = 4.0f;
      float baseAlpha = 0.55f;

      float burstSize = 10.0f;
      float burstAlpha = 0.85f;

      float size = dustBurstActive ? burstSize : baseSize;
      float alpha = dustBurstActive ? burstAlpha : baseAlpha;

      glUniform1f(glGetUniformLocation(particlesRender.id(), "uPointSize"), size);
      glUniform1f(
        glGetUniformLocation(particlesRender.id(), "uDustSizeMul"),
        0.5f
      );
      glUniform1f(glGetUniformLocation(particlesRender.id(), "uAlpha"), alpha);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, sandTex);
      glUniform1i(glGetUniformLocation(particlesRender.id(), "uSandTex"), 0);
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, dustTex);
      glUniform1i(glGetUniformLocation(particlesRender.id(), "uDustTex"), 1);


      particles.drawPoints(particlesRender.id());

      glDepthMask(GL_TRUE);
      glDisable(GL_PROGRAM_POINT_SIZE);
      glDisable(GL_BLEND);


      off.unbind();

      if (rtRecording) {
        ensureFinal(w, h);
        ensureAccum(w, h);

        if (gCollapseStarted && gPhysicsReady) {

          tris.clear();

          for (int g=0; g<6; ++g) {
            for (const glm::mat4& instM : groups[g]) {
              glm::mat4 worldFromModel = instM * preBuildingRT[g];
              appendModelTrianglesWorld(tris, buildings[g], worldFromModel, getTexIndex);
            }
          }

          for (const glm::mat4& instM : streetModels) {
            glm::mat4 worldFromModel = instM * preStreetRT;
            appendModelTrianglesWorld(tris, streetModel, worldFromModel, getTexIndex);
          }

          for (const glm::mat4& instM : groundModelsGLB) {
            glm::mat4 worldFromModel = instM * preGroundRT;
            appendModelTrianglesWorld(tris, groundModel, worldFromModel, getTexIndex);
          }

          appendCollapseChunksRT(tris, collapseModel, physics, getTexIndex);

          rt.triCount = (int)tris.size();
          buildAndUploadBVH(rt, tris);
        }

        glUseProgram(rtPT.id());

        glUniform1i(glGetUniformLocation(rtPT.id(), "uTriCount"),  rt.triCount);
        glUniform1i(glGetUniformLocation(rtPT.id(), "uNodeCount"), rt.nodeCount);

        ensureTexArray(gTexArray, gTexArrayW, gTexArrayH,
                      gTexArrayLayers, texList);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, gTexArray);
        glUniform1i(glGetUniformLocation(rtPT.id(), "uTexArray"), 0);
        glUniform1i(glGetUniformLocation(rtPT.id(), "uTexCount"), gTexArrayLayers);
        glUniform1i(glGetUniformLocation(rtPT.id(), "uManualSRGBDecode"), 1);

        glActiveTexture(GL_TEXTURE31);
        glBindTexture(GL_TEXTURE_CUBE_MAP, sky.cubemap);
        glUniform1i(glGetUniformLocation(rtPT.id(), "uEnv"), 31);
        glUniform1i(glGetUniformLocation(rtPT.id(), "uUseEnv"), 1);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, rt.ssboTris);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, rt.ssboNodes);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, particles.ssbo);
        glUniform1i(glGetUniformLocation(rtPT.id(), "uParticleCount"), particles.count);


        glBindImageTexture(0, finalTex, 0, GL_FALSE, 0,
                          GL_WRITE_ONLY, GL_RGBA16F);

        glUniform2i(glGetUniformLocation(rtPT.id(), "uRes"), w, h);

        glm::mat4 invView = glm::inverse(view);
        glm::mat4 invProj = glm::inverse(proj);
        glUniformMatrix4fv(glGetUniformLocation(rtPT.id(), "uInvView"),
                          1, GL_FALSE, &invView[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(rtPT.id(), "uInvProj"),
                          1, GL_FALSE, &invProj[0][0]);

        glUniform3f(glGetUniformLocation(rtPT.id(), "uCamPos"),
                    cam.pos.x, cam.pos.y, cam.pos.z);

        glm::vec3 sunDirForShading = -sunDir;
        glUniform3f(glGetUniformLocation(rtPT.id(), "uSunDir"),
                    sunDirForShading.x,
                    sunDirForShading.y,
                    sunDirForShading.z);

        glUniform3f(glGetUniformLocation(rtPT.id(), "uSunColor"),
                    sunColor.x * 6.0f, sunColor.y * 6.0f, sunColor.z * 6.0f);

        glUniform1f(glGetUniformLocation(rtPT.id(), "uExposure"), 3.0f);

        static uint32_t seed = 1;
        seed += 1337u;
        glUniform1ui(glGetUniformLocation(rtPT.id(), "uSeed"), seed);

        GLuint gx = (w + 7) / 8;
        GLuint gy = (h + 7) / 8;
        glDispatchCompute(gx, gy, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        glFinish();

        char filename[256];
        snprintf(filename, sizeof(filename),
                "frames/frame_%04d.png", rtFrame);

        saveColor_PNG(finalTex, w, h, filename);
        std::cout << "[RT] Saved " << filename << "\n";

        rtFrame++;

        if (rtCollapseSeen && rtAfterTimer >= 15.0f) {
          rtRecording = false;
          std::cout << "[RT] Done. Creating video...\n";

          system(
            "ffmpeg -y -framerate 20 "
            "-i frames/frame_%04d.png "
            "-c:v libx264 -pix_fmt yuv420p "
            "-crf 18 -preset slow "
            "city_pathtrace.mp4"
          );

          std::cout << "[RT] Video saved: city_pathtrace.mp4\n";
        }
      }

      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glViewport(0, 0, w, h);
      glDisable(GL_SCISSOR_TEST);
      glScissor(0, 0, w, h);
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      glDisable(GL_DEPTH_TEST);
      glDisable(GL_CULL_FACE);

      glBindFramebuffer(GL_READ_FRAMEBUFFER, off.fbo);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
      glBlitFramebuffer(
        0, 0, w, h,
        0, 0, w, h,
        GL_COLOR_BUFFER_BIT, GL_NEAREST
      );

      glEnable(GL_DEPTH_TEST);
      glEnable(GL_CULL_FACE);
    }
    // ======================= GEOMETRY DEMO WINDOW =======================
    if (geoMode && geoWin) {
      if (glfwWindowShouldClose(geoWin)) {
        glfwDestroyWindow(geoWin);
        geoWin = nullptr;
        geoMode = false;
      } else {
        glfwMakeContextCurrent(geoWin);

        int gw, gh;
        glfwGetFramebufferSize(geoWin, &gw, &gh);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, gw, gh);
        glClearColor(0.05f, 0.05f, 0.06f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (geoDirty && geoGpuReady) {
          gGeoLap = geomDemoBaseGeo;
          gGeoLap.applyLaplacian(geomDemo.iters, geomDemo.lambda);
          gGeoLap.setSolidColor(glm::vec3(0.2f, 1.0f, 0.2f));
          gGeoLap.uploadToGPU();
          gVolLap = computeAABBVolume(gGeoLap.pos);

          gGeoTaubin = geomDemoBaseGeo;
          float mu = -(geomDemo.lambda + 0.02f);
          gGeoTaubin.applyTaubin(geomDemo.iters, geomDemo.lambda, mu);
          gGeoTaubin.setSolidColor(glm::vec3(1.0f, 0.7f, 0.2f));
          gGeoTaubin.uploadToGPU();
          gVolTaubin = computeAABBVolume(gGeoTaubin.pos);

          gGeoCurv = geomDemoBaseGeo;
          gGeoCurv.computeCurvatureColors();
          gGeoCurv.uploadToGPU();
          gVolCurv = computeAABBVolume(gGeoCurv.pos);

          gGeoCacheReady = true;
          geoDirty = false;
        }

        auto drawGeoView = [&](int vx, int vy, int vw, int vh, int mode) {
          glViewport(vx, vy, vw, vh);

          float a = float(vw) / float(vh);
          gGeoTarget = glm::vec3(gCollapseStaticM[3]);

          float yaw   = glm::radians(gGeoYawDeg);
          float pitch = glm::radians(gGeoPitchDeg);

          glm::vec3 dir;
          dir.x = cosf(pitch) * cosf(yaw);
          dir.y = sinf(pitch);
          dir.z = cosf(pitch) * sinf(yaw);

          glm::vec3 eye = gGeoTarget + (-dir) * gGeoDist;

          glm::mat4 viewV = glm::lookAt(eye, gGeoTarget, glm::vec3(0,1,0));
          glm::mat4 projV = glm::perspective(glm::radians(50.0f), a, 0.01f, 200.0f);

          glm::mat4 VP  = projV * viewV;
          glm::mat4 M   = gCollapseStaticM;
          glm::mat4 MVP = VP * M;

          textured.use();
          textured.setMat4("uViewProj", &VP[0][0]);
          textured.setMat4("uLightViewProj", &lightViewProj[0][0]);
          textured.setMat4("uPre", &gCollapseStaticM[0][0]);

          textured.setVec3("uSunDir", sunDirForShading.x, sunDirForShading.y, sunDirForShading.z);
          textured.setVec3("uSunColor", sunColor.x, sunColor.y, sunColor.z);
          textured.setVec3("uSkyColor", skyColor.x, skyColor.y, skyColor.z);
          textured.setFloat("uAmbient", ambient);
          textured.setVec3("uCamPos", eye.x, eye.y, eye.z);

          glActiveTexture(GL_TEXTURE2);
          glBindTexture(GL_TEXTURE_2D, shadowMap.depthTex);
          glUniform1i(glGetUniformLocation(textured.id(), "uShadowMap"), 2);
          glUniform2f(glGetUniformLocation(textured.id(), "uShadowMapSize"),
                      (float)shadowMap.size, (float)shadowMap.size);

          for (auto& mesh : collapseModelGeo.meshes) {
            if (mesh.indexCount == 0) continue;
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.albedoTex ? mesh.albedoTex : 0);
            glUniform1i(glGetUniformLocation(textured.id(), "uAlbedo"), 0);
            collapseStaticInst.bindToVAO(mesh.vao, 3);
            glBindVertexArray(mesh.vao);
            glDrawElementsInstanced(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0, 1);
          }
          glBindVertexArray(0);

          if (mode != 1) {
            geomDemoGeo = geomDemoBaseGeo;

            if (mode == 2) {
              geomDemoGeo.applyLaplacian(geomDemo.iters, geomDemo.lambda);
              geomDemoGeo.setSolidColor(glm::vec3(0.2f, 1.0f, 0.2f));
            } else if (mode == 3) {
              float mu = -(geomDemo.lambda + 0.02f);
              geomDemoGeo.applyTaubin(geomDemo.iters, geomDemo.lambda, mu);
              geomDemoGeo.setSolidColor(glm::vec3(1.0f, 0.7f, 0.2f));
            } else if (mode == 4) {
              geomDemoGeo.computeCurvatureColors();
            }

            geomDemoGeo.uploadToGPU();

            glDisable(GL_CULL_FACE);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(1.5f);

            geomDemoShader.use();
            geomDemoShader.setMat4("uMVP", &MVP[0][0]);
            geomDemoGeo.draw();

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_CULL_FACE);
          }
          const char* modeName =
            (mode == 1 ? "Original" :
            (mode == 2 ? "Laplacian" :
            (mode == 3 ? "Taubin" : "Curvature")));

          float vol = 0.f;
          if (mode == 1) vol = gBaseVolGeo;
          else vol = computeAABBVolume(geomDemoGeo.pos);

          float shrinkPct = (gBaseVolGeo > 1e-12f) ? (100.f * (vol / gBaseVolGeo)) : 0.f;
          int pad = 12;
          int lineH = (int)(12.0f * 2.5) + 2;

          int tx = vx + pad;
          int ty = vy + vh - pad - 2 * lineH;

          char line1[256], line2[256];
          snprintf(line1, sizeof(line1), "%s", modeName);

          if (mode == 2) {
            snprintf(line2, sizeof(line2), "iters=%d  lambda=%.2f  Vol=%.6g (%.1f%%)",
                    geomDemo.iters, geomDemo.lambda, vol, shrinkPct);
          } else if (mode == 3) {
            float mu = -(geomDemo.lambda + 0.02f);
            snprintf(line2, sizeof(line2), "iters=%d  lambda=%.2f  mu=%.2f  Vol=%.6g (%.1f%%)",
                    geomDemo.iters, geomDemo.lambda, mu, vol, shrinkPct);
          } else if (mode == 4) {
            snprintf(line2, sizeof(line2), "curvature colors  Vol=%.6g (%.1f%%)", vol, shrinkPct);
          } else {
            snprintf(line2, sizeof(line2), "Vol=%.6g (100%%)", vol);
          }

          drawText2D(gw, gh, tx, ty,     line1, 1.f, 1.f, 1.f);
          drawText2D(gw, gh, tx, ty + lineH, line2, 0.9f, 0.9f, 0.9f);
        };

        int hw = gw / 2;
        int hh = gh / 2;

        drawGeoView(0,   hh, hw, gh - hh, 1);
        drawGeoView(hw,  hh, gw - hw, gh - hh, 2);
        drawGeoView(0,   0,  hw, hh, 3);
        drawGeoView(hw,  0,  gw - hw, hh, 4);


        char title[256];
        snprintf(title, sizeof(title),
                "Geometry Demo - [G close] [2 Laplacian] [3 Taubin] [4 Curvature] | iters=%d lambda=%.2f",
                geomDemo.iters, geomDemo.lambda);
        glfwSetWindowTitle(geoWin, title);

        glfwSwapBuffers(geoWin);

        glfwMakeContextCurrent(win);
      }
    }

    if (!pauseMain) glfwSwapBuffers(win);
  }


  glfwDestroyWindow(win);
  glfwTerminate();
  return 0;
}
