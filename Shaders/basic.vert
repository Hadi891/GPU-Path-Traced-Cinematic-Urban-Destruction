#version 430 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aCol;

// instanced model matrix
layout(location=2) in vec4 iM0;
layout(location=3) in vec4 iM1;
layout(location=4) in vec4 iM2;
layout(location=5) in vec4 iM3;

out vec3 vCol;
out vec3 vWorldPos;
out vec3 vWorldN;
out vec4 vShadowPos;

uniform mat4 uViewProj;
uniform mat4 uLightViewProj;

void main() {
  mat4 M = mat4(iM0, iM1, iM2, iM3);

  vec4 wpos = M * vec4(aPos, 1.0);
  vWorldPos = wpos.xyz;

  // cheap normal (we’ll replace later when real meshes have normals)
  vec3 approxN = normalize(aPos);
  vWorldN = normalize(mat3(M) * approxN);

  vShadowPos = uLightViewProj * wpos;

  vCol = aCol;
  gl_Position = uViewProj * wpos;
}
