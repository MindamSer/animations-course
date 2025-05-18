#include "SDL2/SDL_events.h"
#include "character.h"
#include "import/model.h"
#include "render/mesh.h"
#include "scene.h"
#include <memory>
#include "blend_space_1d.h"
#include "blend_space_2d.h"
#include "single_animation.h"


static glm::mat4 get_projective_matrix()
{
  const float fovY = 90.f * DegToRad;
  const float zNear = 0.01f;
  const float zFar = 500.f;
  return glm::perspective(fovY, engine::get_aspect_ratio(), zNear, zFar);
}

void application_init(Scene &scene)
{
  scene.light.lightDirection = glm::normalize(glm::vec3(-1, -1, 0));
  scene.light.lightColor = glm::vec3(1.f);
  scene.light.ambient = glm::vec3(0.2f);
  scene.userCamera.projection = get_projective_matrix();

  engine::onWindowResizedEvent += [&](const std::pair<int, int> &) { scene.userCamera.projection = get_projective_matrix(); };

  ArcballCamera &cam = scene.userCamera.arcballCamera;
  cam.curZoom = cam.targetZoom = 0.5f;
  cam.maxdistance = 5.f;
  cam.distance = cam.curZoom * cam.maxdistance;
  cam.lerpStrength = 10.f;
  cam.mouseSensitivity = 0.5f;
  cam.wheelSensitivity = 0.05f;
  cam.targetPosition = glm::vec3(0.f, 1.f, 0.f);
  cam.targetRotation = cam.curRotation = glm::vec2(DegToRad * -90.f, DegToRad * -30.f);
  cam.rotationEnable = false;

  scene.userCamera.transform = calculate_transform(scene.userCamera.arcballCamera);

  engine::onMouseButtonEvent += [&](const SDL_MouseButtonEvent &e) { arccam_mouse_click_handler(e, scene.userCamera.arcballCamera); };
  engine::onMouseMotionEvent += [&](const SDL_MouseMotionEvent &e) { arccam_mouse_move_handler(e, scene.userCamera.arcballCamera, scene.userCamera.transform); };
  engine::onMouseWheelEvent += [&](const SDL_MouseWheelEvent &e) { arccam_mouse_wheel_handler(e, scene.userCamera.arcballCamera); };

  engine::onKeyboardEvent += [](const SDL_KeyboardEvent &e) { if (e.keysym.sym == SDLK_F5 && e.state == SDL_RELEASED) recompile_all_shaders(); };


  ModelAsset motusManIdle = load_model("resources/Animations/IPC/MOB1_Stand_Relaxed_Idle_IPC.fbx");

  ModelAsset motusManWalkF = load_model("resources/Animations/IPC/MOB1_Walk_F_Loop_IPC.fbx");
  ModelAsset motusManJogF = load_model("resources/Animations/IPC/MOB1_Jog_F_Loop_IPC.fbx");
  ModelAsset motusManRunF = load_model("resources/Animations/IPC/MOB1_Run_F_Loop_IPC.fbx");

  ModelAsset motusManWalkB = load_model("resources/Animations/IPC/MOB1_Walk_B_Loop_IPC.fbx");
  ModelAsset motusManJogB = load_model("resources/Animations/IPC/MOB1_Jog_B_Loop_IPC.fbx");

  ModelAsset motusManWalkL = load_model("resources/Animations/IPC/MOB1_Walk_L_Loop_IPC.fbx");
  ModelAsset motusManJogL = load_model("resources/Animations/IPC/MOB1_Jog_L_Loop_IPC.fbx");
  ModelAsset motusManRunL = load_model("resources/Animations/IPC/MOB1_Run_L_Loop_IPC.fbx");

  ModelAsset motusManWalkR = load_model("resources/Animations/IPC/MOB1_Walk_R_Loop_IPC.fbx");
  ModelAsset motusManJogR = load_model("resources/Animations/IPC/MOB1_Jog_R_Loop_IPC.fbx");
  ModelAsset motusManRunR = load_model("resources/Animations/IPC/MOB1_Run_R_Loop_IPC.fbx");

  ModelAsset ruby = load_model("resources/sketchfab/ruby.fbx");


  auto material = make_material("character", "sources/shaders/character_vs.glsl", "sources/shaders/character_ps.glsl");
  material->set_property("mainTex", create_texture2d("resources/MotusMan_v55/MCG_diff.jpg"));

  {
    AnimationContext motusContext;
    motusContext.setup(motusManIdle.skeleton.ozzSkeleton);

    Character &motusCharacter = scene.characters.emplace_back();
    motusCharacter.name = "MotusMan_v55";
    motusCharacter.transform = glm::identity<glm::mat4>();
    motusCharacter.meshes = motusManIdle.meshes;
    motusCharacter.material = std::move(material);
    motusCharacter.skeleton = motusManIdle.skeleton;
    motusCharacter.animationContext = std::move(motusContext);

    std::vector<AnimationNode2D> nodes = {
      {motusManIdle.animations[0], {0.f, 0.f}},

      {motusManWalkF.animations[0], {1.f, 0.f}},
      {motusManJogF.animations[0], {2.f, 0.f}},
      {motusManRunF.animations[0], {3.f, 0.f}},

      {motusManWalkB.animations[0], {-1.f, 0.f}},
      {motusManJogB.animations[0], {-2.f, 0.f}},

      {motusManWalkL.animations[0], {0.f, 1.f}},
      {motusManJogL.animations[0], {0.f, 2.f}},
      {motusManRunL.animations[0], {0.f, 3.f}},

      {motusManWalkR.animations[0], {0.f, -1.f}},
      {motusManJogR.animations[0], {0.f, -2.f}},
      {motusManRunR.animations[0], {0.f, -3.f}}
    };
    motusCharacter.controllers.push_back(std::make_shared<BlendSpace2D>(nodes));
  }


  auto whiteMaterial = make_material("character", "sources/shaders/character_vs.glsl", "sources/shaders/character_ps.glsl");

  const uint8_t whiteColor[4] = {255, 255, 255, 255};
  whiteMaterial->set_property("mainTex", create_texture2d(whiteColor, 1, 1, 4));

  {
    AnimationContext rubyContext;
    rubyContext.setup(ruby.skeleton.ozzSkeleton);

    Character &rubyCharacter = scene.characters.emplace_back();
    rubyCharacter.name = "Ruby";
    rubyCharacter.transform = glm::translate(glm::identity<glm::mat4>(), glm::vec3(2.f, 0.f, 0.f));
    rubyCharacter.meshes = ruby.meshes;
    rubyCharacter.material = std::move(whiteMaterial);
    rubyCharacter.skeleton = ruby.skeleton;
    rubyCharacter.animationContext = std::move(rubyContext);
    rubyCharacter.controllers.push_back(std::make_shared<SingleAnimation>(ruby.animations[0]));
  }

  scene.models.push_back(std::move(ruby));

  scene.models.push_back(std::move(motusManIdle));

  scene.models.push_back(std::move(motusManWalkF));
  scene.models.push_back(std::move(motusManJogF));
  scene.models.push_back(std::move(motusManRunF));

  scene.models.push_back(std::move(motusManWalkB));
  scene.models.push_back(std::move(motusManJogB));

  scene.models.push_back(std::move(motusManWalkL));
  scene.models.push_back(std::move(motusManJogL));
  scene.models.push_back(std::move(motusManRunL));

  scene.models.push_back(std::move(motusManWalkR));
  scene.models.push_back(std::move(motusManJogR));
  scene.models.push_back(std::move(motusManRunR));


  auto greenMaterial = make_material("grass", "sources/shaders/floor_vs.glsl", "sources/shaders/floor_ps.glsl");

  const uint8_t greenColor[4] = {127, 255, 127, 255};
  greenMaterial->set_property("mainTex", create_texture2d(greenColor, 1, 1, 4));

  scene.staticModels.push_back(StaticModelAsset{
    {make_plane_mesh()},
    std::move(greenMaterial)
  });

  std::fflush(stdout);
}