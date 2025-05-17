
#include "api.h"
#include "glm/fwd.hpp"
#include "scene.h"
#include <string>

void render_character(const Character &character, const mat4 &cameraProjView, vec3 cameraPosition, const DirectionLight &light)
{
  const Material &material = *character.material;
  const Shader &shader = material.get_shader();

  shader.use();
  material.bind_uniforms_to_shader();
  shader.set_mat4x4("ViewProjection", cameraProjView);
  shader.set_vec3("CameraPosition", cameraPosition);
  shader.set_vec3("LightDirection", glm::normalize(light.lightDirection));
  shader.set_vec3("AmbientLight", light.ambient);
  shader.set_vec3("SunLight", light.lightColor);


  std::span<const mat4x4> bindPose = {
    (const glm::mat4x4 *)character.animationContext.worldTransforms.data(),
    character.animationContext.worldTransforms.size()
  };
  std::vector<mat4> skinningMatrices;

  for (const MeshPtr &mesh : character.meshes)
  {
    skinningMatrices.resize(mesh->inversedBindPose.size());

    for(size_t i = 0; i < mesh->inversedBindPose.size(); ++i)
    {
      const std::string &name = mesh->boneNames[i];
      auto it = character.skeleton.nodesMap.find(name);
      if (it == character.skeleton.nodesMap.end())
      {
        engine::error("Bone \"%s\" from mesh \"%s\" not found in skeleton", name.c_str(), mesh->name.c_str());
        skinningMatrices[i] = mat4(1.f);
      }
      else
      {
        skinningMatrices[i] = bindPose[it->second] * mesh->inversedBindPose[i];
      }
    }

    for(auto &matrix : skinningMatrices)
    {
      matrix = character.transform * matrix;
    }
    shader.set_mat4x4("SkinningMatrices", skinningMatrices.data(), skinningMatrices.size());
    render(mesh);
  }
}

void application_render(Scene &scene)
{
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  const float grayColor = 0.3f;
  glClearColor(grayColor, grayColor, grayColor, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


  const mat4 &projection = scene.userCamera.projection;
  const glm::mat4 &transform = scene.userCamera.transform;
  mat4 projView = projection * inverse(transform);

  for (const Character &character : scene.characters)
    render_character(character, projView, glm::vec3(transform[3]), scene.light);
}
