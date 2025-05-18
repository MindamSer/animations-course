#pragma once

#include "animation_controller.h"
#include "engine/import/model.h"
#include "glm/geometric.hpp"
#include "glm/gtx/compatibility.hpp"


struct AnimationNode2D
{
  const AnimationPtr animation;
  glm::float2 parameter = {0.f, 0.f};
};

struct AnimationTriangle
{
  size_t idx1;
  size_t idx2;
  size_t idx3;
};

struct BlendSpace2D final : IAnimationController
{
  std::vector<AnimationNode2D> animations;
  std::vector<AnimationTriangle> triangulation;
  std::vector<float> weights;
  float progress = 0.f;

  BlendSpace2D(const std::vector<AnimationNode2D> & _animations) : animations{_animations}
  {
    {
      glm::float2 minPoint = {0.f, 0.f};
      glm::float2 maxPoint = {0.f, 0.f};
      for (const AnimationNode2D &node : animations)
      {
        const glm::float2 point = node.parameter;

        minPoint = {min(minPoint.x, point.x), min(minPoint.y, point.y)};
        maxPoint = {max(minPoint.x, point.x), max(minPoint.y, point.y)};
      }
      minPoint -= glm::float2{1.f, 1.f};
      maxPoint += glm::float2{1.f, 1.f};

      std::vector<glm::float2> points;
      points.reserve(animations.size() + 3);
      for (const AnimationNode2D &node : animations)
        points.push_back(node.parameter);
      points.push_back(minPoint);
      points.push_back({minPoint.x, 2 * maxPoint.y - minPoint.y});
      points.push_back({2 * maxPoint.x - minPoint.x, minPoint.y});

      triangulation.push_back({
          animations.size(),
          animations.size() + 1,
          animations.size() + 2
      });

      // Bowyer–Watson algorithm implementation
    }

    weights.resize(animations.size());
    set_parameter({0.f, 0.f});
  }

  void set_parameter(glm::float2 parameter)
  {
    weights.assign(animations.size(), 0.f);

    if (animations.empty())
      return;

    for (const AnimationTriangle &triangle : triangulation)
    {
      const glm::float2 p1 = animations[triangle.idx1].parameter;
      const glm::float2 p2 = animations[triangle.idx2].parameter;
      const glm::float2 p3 = animations[triangle.idx3].parameter;

      float S = 0.5 * glm::length(glm::cross(glm::float3(p2 - p1, 0.0), glm::float3(p3 - p1, 0.0)));
      float l1 = 0.5 * glm::length(glm::cross(glm::float3(p2 - parameter, 0.0), glm::float3(p3 - parameter, 0.0))) / S;
      float l2 = 0.5 * glm::length(glm::cross(glm::float3(p1 - parameter, 0.0), glm::float3(p3 - parameter, 0.0))) / S;
      float l3 = 0.5 * glm::length(glm::cross(glm::float3(p1 - parameter, 0.0), glm::float3(p2 - parameter, 0.0))) / S;

      if (l1 + l2 + l3 <= 1.0f)
      {
        weights[triangle.idx1] = l1;
        weights[triangle.idx2] = l2;
        weights[triangle.idx3] = l3;
        break;
      }
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
