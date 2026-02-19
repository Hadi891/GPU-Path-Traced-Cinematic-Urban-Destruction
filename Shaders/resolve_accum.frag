#version 430 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uAccum;
uniform float uInvSpp;

void main(){
  vec3 c = texture(uAccum, vUV).rgb * uInvSpp;

  // simple tonemap + gamma (optional but helps)
  c = c / (c + vec3(1.0));
  c = pow(c, vec3(1.0/2.2));

  FragColor = vec4(c, 1.0);
}
