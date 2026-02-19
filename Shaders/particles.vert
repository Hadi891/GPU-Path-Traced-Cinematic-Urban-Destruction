#version 430 core
layout(location=0) in vec2 aQuadPos; // [-0.5..0.5] quad

struct Particle {
  vec4 posLife;
  vec4 velSeed;
};

layout(std430, binding=0) buffer Particles {
  Particle P[];
};

out float vLife01;

uniform mat4 uView;
uniform mat4 uProj;
uniform vec2 uViewport;    // (width, height)
uniform float uSize;       // world-ish size scale

void main() {
  uint id = uint(gl_InstanceID);

  vec3 pos = P[id].posLife.xyz;
  float life = P[id].posLife.w;

  // camera right/up from view matrix (inverse rotation)
  vec3 camRight = vec3(uView[0][0], uView[1][0], uView[2][0]);
  vec3 camUp    = vec3(uView[0][1], uView[1][1], uView[2][1]);

  // Make size slightly depend on depth so it feels nicer (optional)
  vec4 viewPos = uView * vec4(pos, 1.0);
  float depth = -viewPos.z;
  float size = uSize * clamp(depth * 0.02, 0.6, 2.2);

  vec3 world = pos + camRight * aQuadPos.x * size + camUp * aQuadPos.y * size;

  gl_Position = uProj * uView * vec4(world, 1.0);

  // normalize life for fade (rough)
  vLife01 = clamp(life / 3.5, 0.0, 1.0);
}
