#pragma once
#include "platform/jwin_platform.h"
#include "platform/jfont.h"
#include "commons/random.h"
#include "camera.h"
#include "debug_draw.h"
#include "containers/chunk_list.h"
#include "ecs/entity_components.h"
#include "ecs/systems/system_render.h"

struct world {
  ecs::entity_manager* EntityManager;
  ecs::render::system* RenderSystem;
  chunk_list PositionNodes;
};

struct application_state
{
  b32 Initialized;
  camera Camera;

  random_generator RandomGenerator;

  u32 PhongProgram;
  u32 PhongProgramNoTex;
  u32 PhongProgramTransparent;
  u32 PlaneStarProgram;
  u32 SphereStarProgram;
  u32 SolidColorProgram;
  u32 EruptionBandProgram;
  u32 TransparentCompositionProgram;
  u32 GaussianProgramY;
  u32 GaussianProgramX;
  u32 FontRenterProgram;

  u32 BlitPlane;
  u32 Plane;
  u32 Sphere;
  u32 Triangle;
  u32 Cone;
  u32 Cube;
  u32 Billboard;
  u32 Cylinder;
  u32 CheckerBoardTexture;
  u32 BrickWallTexture;
  u32 FadedRayTexture;
  u32 EarthTexture;
  u32 WhitePixelTexture;
  u32 Skybox;


  u32 MsaaColorTexture;
  u32 MsaaDepthTexture;
  u32 AccumTexture;
  u32 RevealTexture;
  u32 GaussianATexture;
  u32 GaussianBTexture;

  u32 DefaultFrameBuffer;
  u32 MsaaFrameBuffer;
  u32 TransparentFrameBuffer;
  u32 GaussianAFrameBuffer;
  u32 GaussianBFrameBuffer;

  u32 MSAA;
  u32 Width;
  u32 Height;


  debug_application_render_commands* DebugRenderCommands;

  world World;
};

debug_application_render_commands* GlobalDebugRenderCommands = 0;
application_state* GlobalState = 0;