#version 430 core
in vec3 vDir;
out vec4 FragColor;

uniform samplerCube uSky;

void main() {
  vec3 col = texture(uSky, normalize(vDir)).rgb;

  FragColor = vec4(col, 1.0);
}
