#pragma once

#include "animation_controller.h"
#include "engine/import/model.h"


struct AnimationNode1D
{
  const AnimationPtr animation;
  float parameter = 0.f;
};

struct BlendSpace1D final : IAnimationController
{
  std::vector<AnimationNode1D> animations;
  std::vector<float> weights;
  float progress = 0.f;

  BlendSpace1D(const std::vector<AnimationNode1D> & _animations) : animations{_animations}
  {
    weights.resize(animations.size());
    set_parameter(0.f);
  }

  void set_parameter(float parameter)
  {
    weights.assign(animations.size(), 0.f);

    if (animations.empty())
      return;

    if (parameter < animations[0].parameter)
    {
      weights[0] = 1.f;
      return;
    }
    for (size_t i = 0; i < animations.size() - 1; ++i)
    {
      const AnimationNode1D &curNode = animations[i];
      const AnimationNode1D &nextNode = animations[i + 1];
      if (curNode.parameter <= parameter && parameter < nextNode.parameter)
      {
        float t = (parameter - curNode.parameter) / (nextNode.parameter - curNode.parameter);
        weights[i] = 1.f - t;
        weights[i + 1] = t;
      }
    }
    if (parameter >= animations.back().parameter)
    {
      weights.back() = 1.f;
      return;
    }
  }

  void update(float dt) override
  {
    float duration = 0.f;
    for (size_t i = 0; i < animations.size(); ++i)
    {
      duration += weights[i] * animations[i].animation->duration();
    }

    progress += dt / duration;
    if (progress > 1.f)
      progress -= 1.f;
  }

  void collect_animations(std::vector<WeightedAnimation> &out) override
  {
    for (size_t i = 0; i < animations.size(); ++i)
    {
      out.push_back({animations[i].animation, weights[i], progress});
    }
  }
};
