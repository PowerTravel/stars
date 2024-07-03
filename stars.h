#pragma once
#include "platform/jwin_platform.h"
#include "platform/jfont.h"
#include "commons/random.h"
#include "camera.h"

struct application_state
{
  b32 Initialized;
  int TextPixelSize;
  int OnedgeValue;
  float PixelDistanceScale;
  float FontRelativeScale = 1;
  jfont::sdf_font Font;
  camera Camera;

  random_generator RandomGenerator;

  u32 PhongProgram;
  u32 PhongProgramNoTex;
  u32 PhongProgramTransparent;
  u32 PlaneStarProgram;
  u32 SphereStarProgram;
  u32 SolidColorProgram;
  u32 EruptionBandProgram;
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
};

application_state* GlobalState = 0;