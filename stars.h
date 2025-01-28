#pragma once
#include "platform/jwin_platform.h"
#include "platform/jfont.h"
#include "commons/random.h"
#include "camera.h"
#include "debug_draw.h"

struct application_state
{
  b32 Initialized;
  int TextPixelSize;
  int OnedgeValue;
  float PixelDistanceScale;
  float FontRelativeScale = 1;
  jfont::sdf_font Font;
  jfont::sdf_atlas FontAtlas;
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
  u32 FontTexture;

  u32 DefaultFrameBuffer;
  u32 MsaaFrameBuffer;
  u32 TransparentFrameBuffer;
  u32 GaussianAFrameBuffer;
  u32 GaussianBFrameBuffer;

  u32 MSAA;
  u32 Width;
  u32 Height;


  debug_application_render_commands* DebugRenderCommands;
};

debug_application_render_commands* GlobalDebugRenderCommands = 0;
application_state* GlobalState = 0;