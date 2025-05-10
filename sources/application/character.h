#pragma once
#include "engine/3dmath.h"
#include "engine/render/material.h"
#include "engine/render/mesh.h"
#include "glm/fwd.hpp"
#include "import/model.h"

struct SkeletonRuntime
{
  const SkeletonData skeletonData;
  std::vector<mat4> worldTransforms;

  SkeletonRuntime(const SkeletonData &sd) : skeletonData(sd)
  {
    worldTransforms.resize(skeletonData.localTransforms.size(), mat4(1.f));

    for (size_t i = 0; i < worldTransforms.size(); ++i)
    {
      const int &parent = skeletonData.parents[i];
      if(parent != -1)
      {
        worldTransforms[i] = worldTransforms[parent] * skeletonData.localTransforms[i];
      }
      else
      {
        worldTransforms[i] = skeletonData.localTransforms[i];
      }
    }
  }
};

struct Character
{
  std::string name;
  glm::mat4 transform;
  std::vector<MeshPtr> meshes;
  MaterialPtr material;
  SkeletonRuntime skeleton;
};
