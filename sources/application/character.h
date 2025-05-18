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
#include "animation_controller.h"


struct AnimationLayer
{
  AnimationPtr curentAnimation;
  std::vector<ozz::math::SoaTransform> localLayerTransforms;
  std::unique_ptr<ozz::animation::SamplingJob::Context> samplingCache;
  float currentProgress = 0.f;
  float weight = 1.f;
};

struct AnimationContext
{
  SkeletonPtr skeleton;
  std::vector<ozz::math::SoaTransform> localTransforms;
  std::vector<ozz::math::Float4x4> worldTransforms;
  std::vector<AnimationLayer> layers;

  void setup(const SkeletonPtr &_skeleton)
  {
    skeleton = _skeleton;
    worldTransforms.resize(skeleton->num_joints());
    localTransforms.resize(_skeleton->num_soa_joints());
  }

  void add_animation(const AnimationPtr &animation, float progress, float weight = 1.f)
  {
    AnimationLayer &layer = layers.emplace_back();
    layer.localLayerTransforms.resize(skeleton->num_soa_joints());
    layer.curentAnimation = animation;
    layer.currentProgress = progress;
    layer.weight = weight;
    if (!layer.samplingCache)
      layer.samplingCache = std::make_unique<ozz::animation::SamplingJob::Context>(skeleton->num_joints());
    else
      layer.samplingCache->Resize(skeleton->num_joints());
  }

  void clear_animation_layers()
  {
    layers.clear();
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

  std::vector<std::shared_ptr<IAnimationController>> controllers;

  float linearVelocity = 0.f;

  Character() = default;
  Character(Character &&) = default;
  Character &operator=(Character &&) = default;
  Character(const Character &) = delete;
  Character &operator=(const Character &) = delete;
};

// struct ThirdPersonController
// {
//   Character *controlledCharacter;
//   ModelAsset *characterModel;
//   int cur_state = 0;

//   void set_idle()
//   {
//     controlledCharacter->animationContext.curentAnimation = characterModel->animations[0];
//     cur_state = 0;
//   };

//   void set_walk()
//   {
//     controlledCharacter->animationContext.curentAnimation = characterModel->animations[1];
//     cur_state = 1;
//   };

//   void set_run()
//   {
//     if(cur_state == 1)
//     {
//       controlledCharacter->animationContext.curentAnimation = characterModel->animations[2];
//       cur_state = 2;
//     }
//   };
// };
