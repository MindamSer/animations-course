#include "assimp/anim.h"
#include "assimp/quaternion.h"
#include "assimp/vector3.h"
#include "glm/matrix.hpp"
#include "ozz/animation/offline/animation_builder.h"
#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/runtime/animation.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/memory/unique_ptr.h"
#include "render/mesh.h"
#include <memory>
#include <string_view>
#include <vector>
#include <3dmath.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include "engine/api.h"
#include "glad/glad.h"

#include "ozz/base/maths/quaternion.h"
#include "ozz/base/maths/vec_float.h"
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/skeleton_utils.h>

#include "import/model.h"

MeshPtr create_mesh(const aiMesh *mesh)
{
  std::vector<uint32_t> indices;
  std::vector<vec3> vertices;
  std::vector<vec3> normals;
  std::vector<vec2> uv;
  std::vector<vec4> weights;
  std::vector<uvec4> weightsIndex;

  std::vector<mat4> inversedBindPose;
  std::vector<std::string> boneNames;
  std::map<std::string, int> bonesMap;

  int numVert = mesh->mNumVertices;
  int numFaces = mesh->mNumFaces;

  if (mesh->HasFaces())
  {
    indices.resize(numFaces * 3);
    for (int i = 0; i < numFaces; i++)
    {
      assert(mesh->mFaces[i].mNumIndices == 3);
      for (int j = 0; j < 3; j++)
        indices[i * 3 + j] = mesh->mFaces[i].mIndices[j];
    }
  }

  if (mesh->HasPositions())
  {
    vertices.resize(numVert);
    for (int i = 0; i < numVert; i++)
      vertices[i] = to_vec3(mesh->mVertices[i]);
  }

  if (mesh->HasNormals())
  {
    normals.resize(numVert);
    for (int i = 0; i < numVert; i++)
      normals[i] = to_vec3(mesh->mNormals[i]);
  }

  if (mesh->HasTextureCoords(0))
  {
    uv.resize(numVert);
    for (int i = 0; i < numVert; i++)
      uv[i] = to_vec2(mesh->mTextureCoords[0][i]);
  }

  if (mesh->HasBones())
  {
    weights.resize(numVert, vec4(0.f));
    weightsIndex.resize(numVert);
    int numBones = mesh->mNumBones;
    std::vector<int> weightsOffset(numVert, 0);
    for (int i = 0; i < numBones; i++)
    {
      const aiBone *bone = mesh->mBones[i];

      glm::mat4 mOffsetMatrix;
      memcpy(&mOffsetMatrix, &bone->mOffsetMatrix,sizeof(mOffsetMatrix));
      mOffsetMatrix = glm::transpose(mOffsetMatrix);

      inversedBindPose.push_back(mOffsetMatrix);
      boneNames.push_back(bone->mName.C_Str());
      bonesMap[boneNames.back()] = i;

      for (unsigned j = 0; j < bone->mNumWeights; j++)
      {
        int vertex = bone->mWeights[j].mVertexId;
        int offset = weightsOffset[vertex]++;
        weights[vertex][offset] = bone->mWeights[j].mWeight;
        weightsIndex[vertex][offset] = i;
      }
    }
    // the sum of weights not 1
    for (int i = 0; i < numVert; i++)
    {
      vec4 w = weights[i];
      float s = w.x + w.y + w.z + w.w;
      weights[i] *= 1.f / s;
    }
  }
  return create_mesh(mesh->mName.C_Str(), indices, vertices, normals, uv, weights, weightsIndex, std::move(inversedBindPose), std::move(boneNames), std::move(bonesMap));
}

using RawSkeleton = ozz::animation::offline::RawSkeleton;
using Joint = ozz::animation::offline::RawSkeleton::Joint;

static void load_skeleton(Joint &joint, SkeletonData &skeleton, const aiNode *node, int parent, int depth)
{
  const int curNodeIndex = skeleton.names.size();

  skeleton.names.push_back(node->mName.C_Str());
  joint.name = node->mName.C_Str();

  aiVector3D scaling;
  aiQuaternion rotation;
  aiVector3D position;
  node->mTransformation.Decompose(scaling, rotation, position);

  joint.transform.translation = ozz::math::Float3(position.x, position.y, position.z);
  joint.transform.rotation = ozz::math::Quaternion(rotation.x, rotation.y, rotation.z, rotation.w);
  joint.transform.scale = ozz::math::Float3(scaling.x, scaling.y, scaling.z);

  glm::mat4 localTransform;
  memcpy(&localTransform, &node->mTransformation,sizeof(localTransform));
  localTransform = glm::transpose(localTransform);
  skeleton.localTransforms.push_back(localTransform);

  skeleton.parents.push_back(parent);
  skeleton.depth.push_back(depth);

  joint.children.resize(node->mNumChildren);
  for (int i = 0; i < node->mNumChildren; ++i)
  {
    load_skeleton(joint.children[i], skeleton, node->mChildren[i], curNodeIndex, depth + 1);
  }

  for (size_t i = 0; i < skeleton.names.size(); ++i)
  {
    skeleton.nodesMap[skeleton.names[i]] = i;
  }
}

AnimationPtr create_animation(const aiAnimation *animation, const SkeletonPtr &skeleton)
{
  ozz::animation::offline::RawAnimation rawAnimation;

  rawAnimation.name = animation->mName.C_Str();
  rawAnimation.duration = animation->mDuration / animation->mTicksPerSecond;
  rawAnimation.tracks.resize(skeleton->num_joints());

  for (size_t k = 0; k < skeleton->num_joints(); ++k)
  {
    std::string_view jointName = skeleton->joint_names()[k];
    int channelIdx = -1;
    for (size_t i = 0; i < animation->mNumChannels; ++i)
    {
      const aiNodeAnim &channel = *animation->mChannels[i];
      if (jointName == channel.mNodeName.C_Str())
      {
        channelIdx = i;
        break;
      }
    }

    ozz::animation::offline::RawAnimation::JointTrack &track = rawAnimation.tracks[k];

    if (channelIdx == -1)
    {
      ozz::math::Transform transform = ozz::animation::GetJointLocalRestPose(*skeleton, k);

      track.translations.resize(1);
      track.translations[0].time = 0.f;
      track.translations[0].value = transform.translation;

      track.rotations.resize(1);
      track.rotations[0].time = 0.f;
      track.rotations[0].value = transform.rotation;

      track.scales.resize(1);
      track.scales[0].time = 0.f;
      track.scales[0].value = transform.scale;
    }
    else
    {
      const aiNodeAnim &channel = *animation->mChannels[channelIdx];
      // fill translations
      track.translations.resize(channel.mNumPositionKeys);
      for (size_t j = 0; j < channel.mNumPositionKeys; ++j)
      {
        const aiVectorKey &key = channel.mPositionKeys[j];
        const float mTime = key.mTime / animation->mTicksPerSecond;
        track.translations[j].time = mTime;
        track.translations[j].value = ozz::math::Float3(key.mValue.x, key.mValue.y, key.mValue.z);
        assert (mTime >= 0.f && mTime <= rawAnimation.duration);
        if (j > 0)
          assert(key.mTime >= channel.mPositionKeys[j-1].mTime);
      }

      // fill rotations
      track.rotations.resize(channel.mNumRotationKeys);
      for (size_t j = 0; j < channel.mNumRotationKeys; ++j)
      {
        const aiQuatKey &key = channel.mRotationKeys[j];
        const float mTime = key.mTime / animation->mTicksPerSecond;
        track.rotations[j].time = mTime;
        track.rotations[j].value = ozz::math::Quaternion(key.mValue.x, key.mValue.y, key.mValue.z, key.mValue.w);
        assert (mTime >= 0.f && mTime <= rawAnimation.duration);
        if (j > 0)
          assert(key.mTime >= channel.mRotationKeys[j-1].mTime);
      }

      // fill translations
      track.scales.resize(channel.mNumScalingKeys);
      for (size_t j = 0; j < channel.mNumScalingKeys; ++j)
      {
        const aiVectorKey &key = channel.mScalingKeys[j];
        const float mTime = key.mTime / animation->mTicksPerSecond;
        track.scales[j].time = mTime;
        track.scales[j].value = ozz::math::Float3(key.mValue.x, key.mValue.y, key.mValue.z);
        assert (mTime >= 0.f && mTime <= rawAnimation.duration);
        if (j > 0)
          assert(key.mTime >= channel.mScalingKeys[j-1].mTime);
      }
    }
  }

  assert(rawAnimation.Validate());

  ozz::animation::offline::AnimationBuilder builder;
  std::shared_ptr<ozz::animation::Animation> resAnimation = builder(rawAnimation);

  engine::log("Animation \"%s\" loaded", animation->mName.C_Str());

  return resAnimation;
}

ModelAsset load_model(const char *path)
{

  Assimp::Importer importer;
  importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
  importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 1.f);

  importer.ReadFile(path,
    aiPostProcessSteps::aiProcess_Triangulate |
    aiPostProcessSteps::aiProcess_LimitBoneWeights |
    aiPostProcessSteps::aiProcess_GenNormals |
    aiPostProcessSteps::aiProcess_GlobalScale |
    aiPostProcessSteps::aiProcess_FlipWindingOrder);

  const aiScene *scene = importer.GetScene();
  ModelAsset model;
  model.path = path;
  if (!scene)
  {
    engine::error("Filed to read model file \"%s\"", path);
    return model;
  }

  RawSkeleton rawSkeleton;
  rawSkeleton.roots.resize(1);

  load_skeleton(rawSkeleton.roots[0], model.skeleton, scene->mRootNode, -1, 0);
  assert(rawSkeleton.Validate());

  ozz::animation::offline::SkeletonBuilder builder;
  model.skeleton.ozzSkeleton = builder(rawSkeleton);

  model.meshes.resize(scene->mNumMeshes);
  for (uint32_t i = 0; i < scene->mNumMeshes; i++)
  {
    model.meshes[i] = create_mesh(scene->mMeshes[i]);
  }

  model.animations.resize(scene->mNumAnimations);
  for (uint32_t i = 0; i < scene->mNumAnimations; i++)
  {
    model.animations[i] = create_animation(scene->mAnimations[i], model.skeleton.ozzSkeleton);
  }

  engine::log("Model \"%s\" loaded", path);
  return model;
}