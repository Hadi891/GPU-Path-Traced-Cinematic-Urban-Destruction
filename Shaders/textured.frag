#version 430 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;
in vec4 vLightSpacePos;

out vec4 FragColor;

uniform sampler2D uAlbedo;

uniform vec3 uSunDir;
uniform vec3 uSunColor;
uniform vec3 uSkyColor;
uniform float uAmbient;
uniform vec3 uCamPos;
uniform sampler2D uShadowMap;
uniform vec2 uShadowMapSize;

float shadowFactor(vec4 lightSpacePos, vec3 N, vec3 L)
{
  vec3 proj = lightSpacePos.xyz / lightSpacePos.w;
  proj = proj * 0.5 + 0.5;

  if (proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0) return 1.0;
  if (proj.z > 1.0) return 1.0;

  float current = proj.z;

  float bias = max(0.003 * (1.0 - dot(N, L)), 0.0015);
  
  vec2 texel = 1.0 / uShadowMapSize;
  float sum = 0.0;

  for (int y=-1; y<=1; ++y)
  for (int x=-1; x<=1; ++x) {
    float depth = texture(uShadowMap, proj.xy + vec2(x,y)*texel).r;
    sum += (current - bias <= depth) ? 1.0 : 0.0;
  }
  return sum / 9.0;
}


void main() {
  vec3 albedo = texture(uAlbedo, vUV).rgb;

  vec3 N = normalize(vNormal);
  vec3 L = normalize(uSunDir);

  float ndotl = max(dot(N, L), 0.0);

  float sh = shadowFactor(vLightSpacePos, N, L);
  sh = mix(0.1, 1.0, sh);

  vec3 direct  = uSunColor * ndotl * sh;
  float hemi = clamp(N.y * 0.5 + 0.5, 0.0, 1.0);
  vec3 ambient = uSkyColor * (0.08 + 0.22 * hemi) * uAmbient;

  vec3 V = normalize(uCamPos - vWorldPos);
  vec3 H = normalize(L + V);
  float spec = pow(max(dot(N, H), 0.0), 64.0) * 0.08;
  vec3 specular = uSunColor * spec * sh;

  vec3 color = albedo * (direct + ambient) + specular;

  FragColor = vec4(color, 1.0);
}