#version 430 core

layout(location=0) in vec3 aPos;

layout(location=3) in vec4 iM0;
layout(location=4) in vec4 iM1;
layout(location=5) in vec4 iM2;
layout(location=6) in vec4 iM3;

uniform mat4 uPre;
uniform mat4 uLightViewProj;

void main() {
  mat4 M = mat4(iM0, iM1, iM2, iM3);
  mat4 Model = M * uPre;
  gl_Position = uLightViewProj * (Model * vec4(aPos, 1.0));
}
