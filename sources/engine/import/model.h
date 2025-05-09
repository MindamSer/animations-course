#pragma once
#include "render/mesh.h"
#include <vector>

struct SkeletonData
{
  std::vector<std::string> boneNames;
  std::vector<mat4> boneLocalTransforms;
  std::vector<int> parents;
  std::vector<int> depth;
};

struct ModelAsset
{
  std::string path;
  std::vector<MeshPtr> meshes;
  SkeletonData skeleton;
};

ModelAsset load_model(const char *path);
