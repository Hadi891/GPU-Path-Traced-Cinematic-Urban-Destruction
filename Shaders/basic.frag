#version 430 core
in vec3 vCol;
in vec3 vWorldPos;
in vec3 vWorldN;
in vec4 vShadowPos;

out vec4 FragColor;

uniform vec3 uSunDir;     // direction TO sun (world)
uniform vec3 uSunColor;
uniform vec3 uSkyColor;
uniform float uAmbient;

uniform sampler2D uShadowMap;
uniform vec2 uShadowMapSize; // (width,height)

float shadowPCF(vec4 shadowPos) {
  // perspective divide -> NDC
  vec3 proj = shadowPos.xyz / shadowPos.w;

  // to [0,1]
  vec2 uv = proj.xy * 0.5 + 0.5;
  float z = proj.z * 0.5 + 0.5;

  // outside shadow map => no shadow
  if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) return 0.0;

  // bias (increase when surface faces away from sun)
  vec3 N = normalize(vWorldN);
  vec3 L = normalize(uSunDir);
  float bias = max(0.0008 * (1.0 - dot(N, L)), 0.0003);

  vec2 texel = 1.0 / uShadowMapSize;

  // 3x3 PCF
  float shadow = 0.0;
  for (int y = -1; y <= 1; ++y) {
    for (int x = -1; x <= 1; ++x) {
      float depth = texture(uShadowMap, uv + vec2(x,y) * texel).r;
      shadow += (z - bias > depth) ? 1.0 : 0.0;
    }
  }
  shadow /= 9.0;
  return shadow; // 0 = lit, 1 = full shadow
}

void main() {
  vec3 N = normalize(vWorldN);
  vec3 L = normalize(uSunDir);

  float ndotl = max(dot(N, L), 0.0);

  float sh = shadowPCF(vShadowPos);

  vec3 direct = uSunColor * ndotl * (1.0 - sh);
  vec3 ambient = uSkyColor * uAmbient;

  vec3 color = vCol * (direct + ambient);
  color *= 0.85;

  FragColor = vec4(color, 1.0);
}
