#pragma once
struct Timeline {
  float t = 0.f;
  bool playing = true;

  void update(float dt) {
    if (playing) t += dt;
  }
};
