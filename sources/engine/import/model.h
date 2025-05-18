#pragma once
#include "render/mesh.h"
#include "render/material.h"
#include <vector>

#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/animation.h>

using SkeletonPtr = std::shared_ptr<ozz::animation::Skeleton>;
using AnimationPtr = std::shared_ptr<ozz::animation::Animation>;


struct SkeletonData
{
  std::vector<std::string> names;
  std::vector<mat4> localTransforms;
  std::vector<int> parents;
  std::vector<int> depth;
  std::map<std::string, int> nodesMap;
  SkeletonPtr ozzSkeleton;
};

struct ModelAsset
{
  std::string path;
  std::vector<MeshPtr> meshes;
  SkeletonData skeleton;
  std::vector<AnimationPtr> animations;
};

struct StaticModelAsset
{
  std::vector<MeshPtr> meshes;
  MaterialPtr material;
};

ModelAsset load_model(const char *path);
