#pragma once
#include "engine/3dmath.h"
#include "engine/render/material.h"
#include "engine/render/mesh.h"
#include "glm/fwd.hpp"
#include "import/model.h"
#include <map>
#include <memory>
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"

struct AnimationContext
{
  SkeletonPtr skeleton;
  AnimationPtr curentAnimation;
  std::vector<ozz::math::SoaTransform> localTransforms;
  std::vector<ozz::math::Float4x4> worldTransforms;
  std::unique_ptr<ozz::animation::SamplingJob::Context> samplingCache;
  float currentProgress = 0.f;

  void setup(const SkeletonPtr &_skeleton)
  {
    skeleton = _skeleton;
    localTransforms.resize(skeleton->num_soa_joints());
    worldTransforms.resize(skeleton->num_joints());
    if (!samplingCache)
      samplingCache = std::make_unique<ozz::animation::SamplingJob::Context>(skeleton->num_joints());
    else
      samplingCache->Resize(skeleton->num_joints());
  }
};

struct Character
{
  std::string name;
  glm::mat4 transform;
  std::vector<MeshPtr> meshes;
  MaterialPtr material;
  SkeletonData skeleton;
  AnimationContext animationContext;
};
