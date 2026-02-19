#version 430 core

in float vLife01;
flat in float vIsDust;
out vec4 FragColor;

uniform sampler2D uSandTex;
uniform vec3 uSandTint;
uniform float uAlpha;
uniform sampler2D uDustTex;
uniform vec3 uDustTint;

void main() {
  vec4 tex = (vIsDust > 0.5)
  ? texture(uDustTex, gl_PointCoord)
  : texture(uSandTex, gl_PointCoord);
  if (tex.a < 0.03) discard;

  float fade = smoothstep(0.0, 0.25, vLife01);

  vec3 col = (vIsDust > 0.5)
    ? tex.rgb * uDustTint
    : tex.rgb * uSandTint;
  float a = tex.a * uAlpha * fade;
  a = pow(a, 0.7); // makes it puffier/less harsh (optional)
  FragColor = vec4(col, a);
}
