#pragma once

#include "engine/import/model.h"


struct WeightedAnimation
{
  const AnimationPtr animation;
  float weight;
  float progress;
};

struct IAnimationController
{
  virtual ~IAnimationController() = default;
  virtual void update(float dt) = 0;
  virtual void collect_animations(std::vector<WeightedAnimation> &out) = 0;
};