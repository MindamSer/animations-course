#include "glm/ext/quaternion_common.hpp"
#include "glm/fwd.hpp"
#include "imgui/imgui.h"
#include "imgui/ImGuizmo.h"

#include "scene.h"
#include "user_camera.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui_internal.h"

#include <format>

static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);

static void show_info()
{
  if (ImGui::Begin("Info"))
  {
    ImGui::Text("ESC - exit");
    ImGui::Text("F5 - recompile shaders");
    ImGui::Text("Left Mouse Button and Wheel - controll camera");
  }
  ImGui::End();
}

static glm::vec2 world_to_screen(const UserCamera &camera, glm::vec3 world_position)
{
  glm::vec4 clipSpace = camera.projection * inverse(camera.transform) * glm::vec4(world_position, 1.f);
  glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;
  glm::vec2 screen = (glm::vec2(ndc) + 1.f) / 2.f;
  ImGuiIO &io = ImGui::GetIO();
  return glm::vec2(screen.x * io.DisplaySize.x, io.DisplaySize.y - screen.y * io.DisplaySize.y);
}

static void manipulate_transform(glm::mat4 &transform, const UserCamera &camera)
{
  ImGuizmo::BeginFrame();
  const glm::mat4 &projection = camera.projection;
  //const glm::mat4 &transform = camera.transform;
  mat4 cameraView = inverse(camera.transform);
  ImGuiIO &io = ImGui::GetIO();
  ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

  glm::mat4 globNodeTm = transform;

  ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(projection), mCurrentGizmoOperation, mCurrentGizmoMode,
                       glm::value_ptr(globNodeTm));

  transform = globNodeTm;
}

static void show_characters(Scene &scene)
{
  if (ImGui::Begin("Scene"))
  {
    // implement showing characters when only one character can be selected
    static uint32_t selectedCharacter = -1u;
    static uint32_t selectedNode = -1u;
    for (size_t i = 0; i < scene.characters.size(); i++)
    {
      Character &character = scene.characters[i];
      ImGui::PushID(i);
      if (ImGui::Selectable(character.name.c_str(), selectedCharacter == i, ImGuiSelectableFlags_AllowDoubleClick))
      {
        selectedCharacter = i;
        if (ImGui::IsMouseDoubleClicked(0))
        {
          scene.userCamera.arcballCamera.targetPosition = vec3(character.transform[3]) + vec3(0, 1, 0);
        }
      }
      if (selectedCharacter == i)
      {
        if(ImGui::SliderFloat("linear velocity", &character.linearVelocity, 0.f, 3.f))
        {

        }

        const float INDENT = 15.0f;
        ImGui::Indent(INDENT);
        ImGui::Text("Meshes: %zu", character.meshes.size());

        const SkeletonData &skeleton = character.skeleton;
        ImGui::Text("Skeleton Nodes: %zu", skeleton.names.size());
        for (int j = 0; j < skeleton.names.size(); ++j)
        {
          if (ImGui::Selectable((std::string(skeleton.depth[j], ' ') + skeleton.names[j]).c_str(), selectedNode == j))
          {
            selectedNode = j;
          }
        }

        ImGui::Unindent(INDENT);
      }
      ImGui::PopID();
    }
    if (selectedCharacter < scene.characters.size())
    {
      Character &character = scene.characters[selectedCharacter];
      if (selectedNode < character.skeleton.names.size())
      {
        glm::mat4 &worldTransform = reinterpret_cast<glm::mat4 &>(character.animationContext.worldTransforms[selectedNode]);
        glm::mat4 transform = character.transform * worldTransform;
        manipulate_transform(worldTransform, scene.userCamera);
        worldTransform = inverse(character.transform) * transform;
      }
      else
      {
        manipulate_transform(character.transform, scene.userCamera);
      }

      ImDrawList *drawList = ImGui::GetOverlayDrawList();
      ImColor skeletonColor(255,127,255);
      const float arrowSize = 10.f;
      for (size_t i = 1; i < character.skeleton.names.size(); ++i)
      {
        const int &parent = character.skeleton.parents[i];
        std::vector<glm::mat4> &transforms = reinterpret_cast<std::vector<glm::mat4> &>(character.animationContext.worldTransforms);

        const glm::mat4 fromTransform = character.transform * transforms[parent];
        const glm::mat4 toTransform = character.transform * transforms[i];
        const glm::vec2 fromScreen = world_to_screen(scene.userCamera, glm::vec3(fromTransform[3]));
        const glm::vec2 toScreen = world_to_screen(scene.userCamera, glm::vec3(toTransform[3]));

        drawList->AddLine(fromScreen, toScreen, skeletonColor, 3.f);

        ImVec2 dir(toScreen - fromScreen);
        float d = sqrtf(ImLengthSqr(dir));
        dir /= d;
        ImVec2 ortogonalDir(dir.y, -dir.x);

        drawList->AddTriangleFilled(
          toScreen,
          toScreen - dir * arrowSize + ortogonalDir * 0.5f * arrowSize,
          toScreen - dir * arrowSize - ortogonalDir * 0.5f * arrowSize, skeletonColor);
      }
    }
  }
  ImGui::End();
}

void render_imguizmo(ImGuizmo::OPERATION &mCurrentGizmoOperation, ImGuizmo::MODE &mCurrentGizmoMode)
{
  if (ImGui::Begin("gizmo window"))
  {
    if (ImGui::IsKeyPressed('Z'))
      mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed('E'))
      mCurrentGizmoOperation = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed('R')) // r Key
      mCurrentGizmoOperation = ImGuizmo::SCALE;
    if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
      mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
      mCurrentGizmoOperation = ImGuizmo::ROTATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
      mCurrentGizmoOperation = ImGuizmo::SCALE;

    if (mCurrentGizmoOperation != ImGuizmo::SCALE)
    {
      if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
        mCurrentGizmoMode = ImGuizmo::LOCAL;
      ImGui::SameLine();
      if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
        mCurrentGizmoMode = ImGuizmo::WORLD;
    }
  }
  ImGui::End();
}

static void show_models(Scene &scene)
{
  if (ImGui::Begin("Models"))
  {
    static uint32_t selectedModel = -1u;
    for (size_t i = 0; i < scene.models.size(); i++)
    {
      const ModelAsset &model = scene.models[i];
      if (ImGui::Selectable(model.path.c_str(), selectedModel == i))
      {
        selectedModel = i;
      }

      if (selectedModel == i)
      {
        ImGui::Indent(15.0f);

        ImGui::Text("Path: %s", model.path.c_str());

        if(ImGui::TreeNode(std::format("all_meshes_%d", i).c_str(), "Meshes: %zu", model.meshes.size()))
        {
          for (size_t j = 0; j < model.meshes.size(); j++)
          {
            const MeshPtr &mesh = model.meshes[j];

            ImGui::PushID(j);
            if(ImGui::TreeNode(std::format("cur_mesh_%d", j).c_str(), "%s", mesh->name.c_str()))
            {
              ImGui::Text("Bones :%zu", mesh->boneNames.size());
              int boneIndex = 0;
              for (const std::string &name : mesh->boneNames)
              {
                ImGui::Text("(%d) %s", boneIndex, name.c_str());
                ++boneIndex;
              }

              ImGui::TreePop();
            }
            ImGui::PopID();
          }

          ImGui::TreePop();
        }

        ImGui::Unindent(15.0f);
      }
    }
  }
  ImGui::End();
}

void application_imgui_render(Scene &scene)
{
  render_imguizmo(mCurrentGizmoOperation, mCurrentGizmoMode);

  show_info();
  show_characters(scene);
  show_models(scene);
}