#version 430 core

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNrm;
layout(location=2) in vec2 aUV;

// instanced model matrix (4 vec4 attributes)
layout(location=3) in vec4 iM0;
layout(location=4) in vec4 iM1;
layout(location=5) in vec4 iM2;
layout(location=6) in vec4 iM3;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;
out vec4 vLightSpacePos;

uniform mat4 uPre;
uniform mat4 uViewProj;
uniform mat4 uLightViewProj;


void main() {
  mat4 M = mat4(iM0, iM1, iM2, iM3);

  mat4 Model = M * uPre;

  vec4 wpos = Model * vec4(aPos, 1.0);
  vLightSpacePos = uLightViewProj * wpos;
  vWorldPos = wpos.xyz;

  mat3 normalMat = transpose(inverse(mat3(Model)));
  vNormal = normalize(normalMat * aNrm);

  vUV = aUV;
  gl_Position = uViewProj * wpos;
}