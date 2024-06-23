#pragma once
#include "platform/jwin_platform.h"
#include "platform/jfont.h"
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


  u32 PhongProgram;
  u32 PhongProgramNoTex;
  u32 PlaneStarProgram;
  u32 SphereStarProgram;
  u32 SolidSphereProgram;
  u32 Plane;
  u32 Sphere;
  u32 Triangle;
  u32 Cone;
  u32 Cube;
  u32 Cylinder;
  u32 PlaneTexture;
  u32 SphereTexture;
};

application_state* GlobalState = 0;