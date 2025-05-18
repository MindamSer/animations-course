#pragma once

#include "animation_controller.h"
#include "engine/import/model.h"


struct SingleAnimation final : IAnimationController
{
  const AnimationPtr animation;
  float progress = 0.f;

  SingleAnimation(const AnimationPtr &_animation) : animation{_animation} {}

  void update(float dt) override
  {
    float duration = animation->duration();

    progress += dt / duration;
    if (progress > 1.f)
      progress -= 1.f;
  }

  void collect_animations(std::vector<WeightedAnimation> &out) override
  {
    out.push_back({animation, 1.f, progress});
  }
};
