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

      layer.currentProgress += engine::get_delta_time() / layer.curentAnimation->duration();
      if (layer.currentProgress > 1.f)
        layer.currentProgress -= 1.f;
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