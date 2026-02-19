#version 430 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uSceneColor;
uniform sampler2D uSceneDepth;

uniform mat4 uInvProj;
uniform mat4 uInvView;

uniform vec3 uCamPos;
uniform vec3 uSunDir;     // normalized
uniform vec3 uSunColor;   // warm
uniform float uTime;

// Fog controls
uniform float uFogDensity;     // ~0.03 .. 0.12
uniform float uFogHeight;      // height falloff
uniform float uFogMaxDist;     // raymarch end distance
uniform int   uFogSteps;       // 48..128
uniform float uScattering;     // god ray strength

// Reconstruct world position from depth
vec3 reconstructWorldPos(vec2 uv, float depth01) {
  // NDC
  float z = depth01 * 2.0 - 1.0;
  vec4 clip = vec4(uv * 2.0 - 1.0, z, 1.0);

  vec4 view = uInvProj * clip;
  view /= view.w;

  vec4 world = uInvView * view;
  return world.xyz;
}

float fogDensityAt(vec3 p) {
  // Height-based fog near ground + a tiny animated variation
  float h = max(p.y + 0.5, 0.0); // ground around y ~ -0.5
  float heightTerm = exp(-h * uFogHeight);

  heightTerm = mix(0.35, 1.0, heightTerm);

  // small animated modulation (cheap "turbulence")
  float n = sin(0.15*p.x + 0.08*p.z + 0.6*uTime) * 0.5 + 0.5;

  return uFogDensity * heightTerm * (0.7 + 0.6*n);
}

void main() {
  vec3 scene = texture(uSceneColor, vUV).rgb;
  float depth01 = texture(uSceneDepth, vUV).r;

  // If depth is 1, it's background (no geometry). Still allow a bit of fog.
  vec3 hitWorld = reconstructWorldPos(vUV, depth01);
  float hitDist = length(hitWorld - uCamPos);

  float maxDist = min(hitDist, uFogMaxDist);

  // view ray
  vec3 rayDir = normalize(hitWorld - uCamPos);

  // Raymarch fog
  float steps = float(uFogSteps);
  float dt = maxDist / steps;

  vec3 accum = vec3(0.0);
  float transmittance = 1.0;

  // God rays: we approximate light visibility with a phase function aligned to sun dir
  float phase = pow(max(dot(rayDir, uSunDir), 0.0), 6.0); // tighter beam

  for (int i = 0; i < uFogSteps; ++i) {
    float t = (float(i) + 0.5) * dt;
    vec3 p = uCamPos + rayDir * t;

    float d = fogDensityAt(p);
    float absorb = exp(-d * dt);     // Beer-Lambert
    float scatter = (1.0 - absorb);  // amount scattered toward camera

    vec3 fogTint = vec3(1.0, 0.85, 0.70); // warm mist
    vec3 light = (uSunColor * fogTint) * (uScattering * phase);

    float dist01 = clamp(t / uFogMaxDist, 0.0, 1.0);
    // cool far fog, warm near fog
    vec3 nearFog = vec3(1.0, 0.85, 0.7);
    vec3 farFog  = vec3(0.55, 0.65, 0.8);
    vec3 fogColor = mix(nearFog, farFog, dist01);
    light *= fogColor;


    accum += transmittance * scatter * light;
    transmittance *= absorb;

    if (transmittance < 0.02) break;
  }

  // Composite: scene attenuated by fog + additive scattering
  float minVis = 0.15; // keep ground visible
  float T = max(transmittance, minVis);

  vec3 outCol = scene * T + accum;

  // Simple film-ish exposure + gamma
  float exposure = 1.6;              // 1.2..2.2
  vec3 mapped = vec3(1.0) - exp(-outCol * exposure); // Reinhard-ish exponential
  mapped = pow(mapped, vec3(1.0/2.2));               // gamma

  FragColor = vec4(mapped, 1.0);
}
