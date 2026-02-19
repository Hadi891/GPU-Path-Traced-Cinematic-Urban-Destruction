#version 430 core

struct Particle {
  vec4 posLife;
  vec4 velSeed;
};

layout(std430, binding=0) buffer Particles {
  Particle P[];
};

uniform mat4 uView;
uniform mat4 uProj;
uniform float uPointSize;
uniform float uDustSizeMul;

out float vLife01;
flat out float vIsDust;   // <-- IMPORTANT (see problem 2)

void main() {
  uint id = uint(gl_VertexID);

  vec3 pos = P[id].posLife.xyz;
  float life = P[id].posLife.w;

  // ✅ decide dust FIRST
  vIsDust = (P[id].velSeed.w < 0.0) ? 1.0 : 0.0;

  vec4 viewPos = uView * vec4(pos, 1.0);
  gl_Position = uProj * viewPos;

  float dist = max(1.0, -viewPos.z);
  float size = uPointSize;

  if (vIsDust > 0.5)
    size *= uDustSizeMul;

  gl_PointSize = size / dist;

  vLife01 = clamp(life / 2.0, 0.0, 1.0);
}
