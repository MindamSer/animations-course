#pragma once

#include "animation_controller.h"
#include "engine/import/model.h"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "glm/gtx/compatibility.hpp"
#include "glm/matrix.hpp"


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

inline bool point_D_in_circle_ABC(glm::float2 A, glm::float2 B, glm::float2 C, glm::float2 D)
{
  if (glm::cross(glm::float3(B-A, 0.f), glm::float3(C-A, 0.f)).z < 0.f)
    std::swap(B, C);

  return glm::determinant(glm::mat4x4{
    A.x, A.y, A.x*A.x + A.y*A.y, 1.f,
    B.x, B.y, B.x*B.x + B.y*B.y, 1.f,
    C.x, C.y, C.x*C.x + C.y*C.y, 1.f,
    D.x, D.y, D.x*D.x + D.y*D.y, 1.f
  }) > 0.f;
}

struct BlendSpace2D final : IAnimationController
{
  std::vector<AnimationNode2D> animations;
  std::vector<AnimationTriangle> triangulation;
  std::vector<float> weights;
  float progress = 0.f;

  BlendSpace2D(const std::vector<AnimationNode2D> & _animations) : animations{_animations}
  {
    {
      std::vector<glm::float2> points = {};
      points.reserve(animations.size() + 3);
      for (const AnimationNode2D &node : animations)
        points.push_back(node.parameter);

      {
        glm::float2 minPoint = {0.f, 0.f};
        glm::float2 maxPoint = {0.f, 0.f};
        for (const AnimationNode2D &node : animations)
        {
          const glm::float2 point = node.parameter;

          minPoint = {min(minPoint.x, point.x), min(minPoint.y, point.y)};
          maxPoint = {max(maxPoint.x, point.x), max(maxPoint.y, point.y)};
        }
        minPoint -= glm::float2{1.f, 1.f};
        maxPoint += glm::float2{1.f, 1.f};

        float maxSide = max(maxPoint.x - minPoint.x, maxPoint.y - minPoint.y);
        points.push_back(minPoint);
        points.push_back({minPoint.x, minPoint.y + 2 * maxSide});
        points.push_back({minPoint.x + 2 * maxSide, minPoint.y});
      }

      triangulation = {};
      triangulation.push_back({
          animations.size(),
          animations.size() + 1,
          animations.size() + 2
      });


      std::vector<size_t> trianglesIndiciesToDelete = {};
      for (size_t i = 0; i < points.size(); ++i)
      {
        trianglesIndiciesToDelete.clear();

        const glm::float2 curPoint = points[i];

        for (size_t j = 0; j < triangulation.size(); ++j)
        {
          const AnimationTriangle curTriangle = triangulation[j];

          if (point_D_in_circle_ABC(
            points[curTriangle.idx1],
            points[curTriangle.idx2],
            points[curTriangle.idx3],
            curPoint))
          {
            trianglesIndiciesToDelete.push_back(j);
          }
        }

        for (const size_t deletingTriangleIdx : trianglesIndiciesToDelete)
        {
          const AnimationTriangle curTriangle = triangulation[deletingTriangleIdx];

          triangulation.push_back({i, curTriangle.idx2, curTriangle.idx3});
          triangulation.push_back({curTriangle.idx1, i, curTriangle.idx3});
          triangulation.push_back({curTriangle.idx1, curTriangle.idx2, i});
        }

        for (int j = 0; j < trianglesIndiciesToDelete.size(); ++j)
          std::swap(triangulation[trianglesIndiciesToDelete[j]], *(triangulation.end() - 1 - j));

        triangulation.erase(triangulation.end() - trianglesIndiciesToDelete.size(), triangulation.end());
      }


      trianglesIndiciesToDelete.clear();

      for (size_t i = 0; i < triangulation.size(); ++i)
      {
        const AnimationTriangle curTriangle = triangulation[i];

        if (
          curTriangle.idx1 >= animations.size() ||
          curTriangle.idx2 >= animations.size() ||
          curTriangle.idx3 >= animations.size())
        {
          trianglesIndiciesToDelete.push_back(i);
        }
      }

      for (int i = trianglesIndiciesToDelete.size() - 1; i >= 0; --i)
        triangulation.erase(triangulation.begin() + trianglesIndiciesToDelete[i]);
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
