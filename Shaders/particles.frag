#version 430 core
in float vLife01;
out vec4 FragColor;

void main() {
  // fake round particle using fragment coord inside quad is not available,
  // so we rely on alpha being low and let blending + motion do the job.
  float a = smoothstep(0.0, 0.2, vLife01) * smoothstep(0.0, 0.2, 1.0 - vLife01);

  vec3 col = vec3(0.55);
  FragColor = vec4(col, a * 0.10);
}
