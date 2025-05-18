#include "SDL2/SDL_events.h"
#include "character.h"
#include "import/model.h"
#include "render/mesh.h"
#include "scene.h"


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
  {
    ModelAsset motusManWalk = load_model("resources/Animations/IPC/MOB1_Walk_F_Loop_IPC.fbx");
    ModelAsset motusManRun = load_model("resources/Animations/IPC/MOB1_Run_F_Loop_IPC.fbx");

    motusManIdle.animations.push_back(motusManWalk.animations[0]);
    motusManIdle.animations.push_back(motusManRun.animations[0]);
  }

  ModelAsset ruby = load_model("resources/sketchfab/ruby.fbx");


  auto material = make_material("character", "sources/shaders/character_vs.glsl", "sources/shaders/character_ps.glsl");
  material->set_property("mainTex", create_texture2d("resources/MotusMan_v55/MCG_diff.jpg"));

  {
    AnimationContext motusContext;
    motusContext.setup(motusManIdle.skeleton.ozzSkeleton);
    motusContext.curentAnimation = motusManIdle.animations[0];

    scene.characters.emplace_back(Character{
    "MotusMan_v55",
    glm::identity<glm::mat4>(),
    motusManIdle.meshes,
    std::move(material),
    motusManIdle.skeleton,
    std::move(motusContext)
    });
  }


  auto whiteMaterial = make_material("character", "sources/shaders/character_vs.glsl", "sources/shaders/character_ps.glsl");

  const uint8_t whiteColor[4] = {255, 255, 255, 255};
  whiteMaterial->set_property("mainTex", create_texture2d(whiteColor, 1, 1, 4));

  {
    AnimationContext rubyContext;
    rubyContext.setup(ruby.skeleton.ozzSkeleton);
    rubyContext.curentAnimation = ruby.animations[0];

    scene.characters.emplace_back(Character{
      "Ruby",
      glm::translate(glm::identity<glm::mat4>(), glm::vec3(2.f, 0.f, 0.f)),
      ruby.meshes,
      std::move(whiteMaterial),
      ruby.skeleton,
      std::move(rubyContext)
    });
  }

  scene.models.push_back(std::move(motusManIdle));
  scene.models.push_back(std::move(ruby));


  scene.controller = {
    &scene.characters[0],
    &scene.models[0]
  };
  engine::onKeyboardEvent += [&](const SDL_KeyboardEvent &e) { if (e.keysym.sym == SDLK_w && e.state == SDL_RELEASED) scene.controller.set_idle(); };
  engine::onKeyboardEvent += [&](const SDL_KeyboardEvent &e) { if (e.keysym.sym == SDLK_w && e.state == SDL_PRESSED) scene.controller.set_walk(); };
  engine::onKeyboardEvent += [&](const SDL_KeyboardEvent &e) { if (e.keysym.sym == SDLK_LSHIFT && e.state == SDL_PRESSED) scene.controller.set_run(); };

  auto greenMaterial = make_material("grass", "sources/shaders/floor_vs.glsl", "sources/shaders/floor_ps.glsl");

  const uint8_t greenColor[4] = {127, 255, 127, 255};
  greenMaterial->set_property("mainTex", create_texture2d(greenColor, 1, 1, 4));

  scene.staticModels.push_back(StaticModelAsset{
    {make_plane_mesh()},
    std::move(greenMaterial)
  });

  std::fflush(stdout);
}