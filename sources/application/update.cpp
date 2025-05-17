#include "ozz/animation/runtime/local_to_model_job.h"
#include "scene.h"
#include "character.h"
#include "ozz/base/span.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/animation/runtime/sampling_job.h"
#include <cassert>

void application_update(Scene &scene)
{
  arcball_camera_update(
    scene.userCamera.arcballCamera,
    scene.userCamera.transform,
    engine::get_delta_time());

  for (Character &character : scene.characters)
  {
    AnimationContext &animationContext = character.animationContext;

    if (animationContext.curentAnimation != nullptr)
    {
      ozz::animation::SamplingJob samplingJob;
      samplingJob.ratio = animationContext.currentProgress;
      //assert(samplingJob.ratio >= 0.f && samplingJob.ratio <= 1.f);
      samplingJob.animation = animationContext.curentAnimation.get();
      samplingJob.context = animationContext.samplingCache.get();
      samplingJob.output = ozz::make_span(animationContext.localTransforms);

      assert(samplingJob.Validate());
      const bool success = samplingJob.Run();
      assert(success);

      animationContext.currentProgress += engine::get_delta_time() / animationContext.curentAnimation->duration();
      if (animationContext.currentProgress > 1.f) animationContext.currentProgress -= 1.f;
    }
    else
    {
      const auto &tPose = animationContext.skeleton->joint_rest_poses();
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