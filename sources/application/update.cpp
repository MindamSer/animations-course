#include "animation_controller.h"
#include "blend_space_1d.h"
#include "blend_space_2d.h"
#include "glm/ext/quaternion_geometric.hpp"
#include "ozz/animation/runtime/blending_job.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "scene.h"
#include "character.h"
#include "ozz/base/span.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/animation/runtime/sampling_job.h"
#include <cassert>
#include <cstddef>

void application_update(Scene &scene)
{
  arcball_camera_update(
    scene.userCamera.arcballCamera,
    scene.userCamera.transform,
    engine::get_delta_time());

  for (Character &character : scene.characters)
  {
    AnimationContext &animationContext = character.animationContext;

    std::vector<WeightedAnimation> animations;
    for (auto &controller : character.controllers)
    {
      if (BlendSpace2D *blendSpace = dynamic_cast<BlendSpace2D *>(controller.get()))
      {
        blendSpace -> set_parameter(character.velocity);
      }
      else if (BlendSpace1D *blendSpace = dynamic_cast<BlendSpace1D *>(controller.get()))
      {
        blendSpace -> set_parameter(glm::length(character.linearVelocity));
      }
    }
    for (auto &controller : character.controllers)
    {
      controller->update(engine::get_delta_time());
      controller->collect_animations(animations);
    }

    animationContext.clear_animation_layers();
    for (const WeightedAnimation &wa : animations)
      animationContext.add_animation(wa.animation, wa.progress, wa.weight);

    for (AnimationLayer &layer : animationContext.layers)
    {
      assert(layer.curentAnimation != nullptr);

      ozz::animation::SamplingJob samplingJob;
      samplingJob.ratio = layer.currentProgress;
      samplingJob.animation = layer.curentAnimation.get();
      samplingJob.context = layer.samplingCache.get();
      samplingJob.output = ozz::make_span(layer.localLayerTransforms);

      assert(samplingJob.Validate());
      const bool success = samplingJob.Run();
      assert(success);
    }


    if (!animationContext.layers.empty())
    {
      ozz::animation::BlendingJob blendingJob;
      blendingJob.output = ozz::make_span(animationContext.localTransforms);
      blendingJob.threshold = 0.01f;
      std::vector<ozz::animation::BlendingJob::Layer> layers(animationContext.layers.size());
      for (int i = 0; i < animationContext.layers.size(); ++i)
      {
        layers[i].weight = animationContext.layers[i].weight;
        layers[i].transform = ozz::make_span(animationContext.layers[i].localLayerTransforms);
      }
      blendingJob.layers = ozz::make_span(layers);
      blendingJob.rest_pose = animationContext.skeleton->joint_rest_poses();

      assert(blendingJob.Validate());
      const bool success = blendingJob.Run();
      assert(success);

    }
    else
    {
      auto tPose = animationContext.skeleton->joint_rest_poses();
      animationContext.localTransforms.assign(tPose.begin(), tPose.end());
    }

    ozz::animation::LocalToModelJob localToModelJob;
    localToModelJob.skeleton = animationContext.skeleton.get();
    localToModelJob.input = ozz::make_span(animationContext.localTransforms);
    localToModelJob.output = ozz::make_span(animationContext.worldTransforms);

    assert(localToModelJob.Validate());
    const bool success = localToModelJob.Run();
    assert(success);
  }
}