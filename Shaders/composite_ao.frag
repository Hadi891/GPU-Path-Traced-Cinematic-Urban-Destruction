#version 430 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uSceneColor;
uniform sampler2D uAO;

void main(){
  // Fix horizontal mirroring coming from fullscreen quad UVs
  //vec2 uv = vec2(1.0 - vUV.x, vUV.y);

  vec3 col = texture(uSceneColor, vUV).rgb;

  // AO: you already needed vertical flip, keep it, but use corrected uv.x too
  // vec2 uvAO = vec2(uv.x, 1.0 - uv.y);
  float ao = texture(uAO, vUV).r;

  ao = clamp(ao, 0.35, 1.0);
  col *= ao;
  FragColor = vec4(col, 1.0);
}
