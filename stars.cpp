#include "stars.h"

#include "render_utils.h"


#include "platform/jwin_platform_memory.cpp"
#include "platform/jwin_platform_input.h"
#include "platform/jfont.cpp"
#include "platform/obj_loader.cpp"
#include "platform/text_input.cpp"
#include "renderer/software_render_functions.cpp"
#include "renderer/render_push_buffer/application_render_push_buffer.cpp"
#include "math/AABB.cpp"
#include "camera.cpp"
#include "math/geometry_math.h"
#include "skybox_drawing.h"
#include "containers/chunk_list.cpp"
#include "containers/linked_memory.cpp"
#include "ecs/entity_components_backend.cpp"
#include "ecs/entity_components.cpp"
#include "ecs/systems/system_position.cpp"
#include "ecs/systems/system_render.cpp"
#include "menu/menu_interface.cpp"
#include "imgui/imgui.cpp"
#include "imgui/application_imgui.cpp"

#include "utils.h"

#define SPOTCOUNT 200

global_variable r32 g_t = 0;

u32 CreatePhongProgram(render_group* RenderGroup)
{
  u32 ProgramHandle = NewShaderProgram(RenderGroup, "PhongShading");
  AddUniform(RenderGroup, UniformType::M4,  ProgramHandle, "ProjectionMat");
  AddUniform(RenderGroup, UniformType::M4,  ProgramHandle, "ModelView");
  AddUniform(RenderGroup, UniformType::M4,  ProgramHandle, "NormalView");
  AddUniform(RenderGroup, UniformType::V3,  ProgramHandle, "LightDirection");
  AddUniform(RenderGroup, UniformType::V3,  ProgramHandle, "LightColor");
  AddUniform(RenderGroup, UniformType::V4,  ProgramHandle, "MaterialAmbient");
  AddUniform(RenderGroup, UniformType::V4,  ProgramHandle, "MaterialDiffuse");
  AddUniform(RenderGroup, UniformType::V4,  ProgramHandle, "MaterialSpecular");
  AddUniform(RenderGroup, UniformType::R32, ProgramHandle, "Shininess");
  CompileShader(RenderGroup, ProgramHandle,
      1, LoadFileFromDisk("..\\jwin\\shaders\\PhongVertexCameraView.glsl"),
      1, LoadFileFromDisk("..\\jwin\\shaders\\PhongFragmentCameraView.glsl"));
  return ProgramHandle;
}


u32 CreatePhongTransparentProgram(render_group* RenderGroup)
{
  u32 ProgramHandle = NewShaderProgram(RenderGroup,"PhongShadingTransparent");

  AddUniform(RenderGroup, UniformType::M4, ProgramHandle, "ProjectionMat");
  AddUniform(RenderGroup, UniformType::M4, ProgramHandle, "ModelView");
  AddUniform(RenderGroup, UniformType::M4, ProgramHandle, "NormalView");
  AddUniform(RenderGroup, UniformType::V3, ProgramHandle, "LightDirection");
  AddUniform(RenderGroup, UniformType::V3, ProgramHandle, "LightColor");
  AddUniform(RenderGroup, UniformType::V4, ProgramHandle, "MaterialAmbient");
  AddUniform(RenderGroup, UniformType::V4, ProgramHandle, "MaterialDiffuse");
  AddUniform(RenderGroup, UniformType::V4, ProgramHandle, "MaterialSpecular");
  AddUniform(RenderGroup, UniformType::R32, ProgramHandle, "Shininess");
  CompileShader(RenderGroup, ProgramHandle,
      1, LoadFileFromDisk("..\\jwin\\shaders\\PhongVertexCameraViewTransparent.glsl"),
      1, LoadFileFromDisk("..\\jwin\\shaders\\PhongFragmentCameraViewTransparent.glsl"));
  return ProgramHandle;
}

u32 CreatePlaneStarProgram(render_group* RenderGroup)
{
  u32 ProgramHandle = NewShaderProgram(RenderGroup, "StarPlane");
  AddUniform(RenderGroup, UniformType::M4,  ProgramHandle, "ProjectionMat");
  AddUniform(RenderGroup, UniformType::M4,  ProgramHandle, "ViewMat");
  AddVarying(RenderGroup, UniformType::M4,  ProgramHandle, "ModelMat");
  AddVarying(RenderGroup, UniformType::V4,  ProgramHandle, "Color");
  AddVarying(RenderGroup, UniformType::R32, ProgramHandle, "Radius");
  AddVarying(RenderGroup, UniformType::R32, ProgramHandle, "FadeDist");
  AddVarying(RenderGroup, UniformType::V3,  ProgramHandle, "Center");
  CompileShader(RenderGroup, ProgramHandle,
      1, LoadFileFromDisk("..\\jwin\\shaders\\StarPlaneVertex.glsl"),
      1, LoadFileFromDisk("..\\jwin\\shaders\\StarPlaneFragment.glsl"));
  return ProgramHandle;
}

u32 CreateSolidColorProgram(render_group* RenderGroup)
{
  u32 ProgramHandle = NewShaderProgram(RenderGroup, "SolidColor");
  AddUniform(RenderGroup, UniformType::M4, ProgramHandle, "ProjectionMat");
  AddUniform(RenderGroup, UniformType::M4, ProgramHandle, "ModelView");
  AddUniform(RenderGroup, UniformType::V4, ProgramHandle, "Color");
  CompileShader(RenderGroup, ProgramHandle,
     1, LoadFileFromDisk("..\\jwin\\shaders\\SolidColorVertex.glsl"),
     1, LoadFileFromDisk("..\\jwin\\shaders\\SolidColorFragment.glsl"));
  return ProgramHandle;
}


u32 CreateEruptionBandProgram(render_group* RenderGroup)
{
  u32 ProgramHandle = NewShaderProgram(RenderGroup, "EruptionBand");
  AddUniform(RenderGroup, UniformType::M4,  ProgramHandle, "ProjectionMat");
  AddUniform(RenderGroup, UniformType::M4,  ProgramHandle, "ModelView");
  AddVarying(RenderGroup, UniformType::V4,  ProgramHandle, "Color");
  AddVarying(RenderGroup, UniformType::V3,  ProgramHandle, "Center");
  AddVarying(RenderGroup, UniformType::R32, ProgramHandle, "InnerRadii");
  AddVarying(RenderGroup, UniformType::R32, ProgramHandle, "OuterRadii");
  CompileShader(RenderGroup, ProgramHandle,
     1, LoadFileFromDisk("..\\jwin\\shaders\\EruptionBandVertex.glsl"),
     1, LoadFileFromDisk("..\\jwin\\shaders\\EruptionBandFragment.glsl"));
  return ProgramHandle;
}



struct eurption_band{
  v4 Color;
  v3 Center;
  r32 InnerRadii;
  r32 OuterRadii;
};


struct ray_cast
{
  m4 ModelMat;
  v4 Color;
  r32 Radius;
  r32 FaceDist;
  v3 Center;
};

ray_cast CastRay(camera* Camera, r32 Angle, r32 Width, r32 Length, v4 Color, v3 Position)
{
  ray_cast Result = {};
  v3 Forward, Up, Right;
  GetCameraDirections(Camera, &Up, &Right, &Forward);
  v3 Direction = GetCameraPosition(Camera) - Position;
  m4 StaticAngle = GetRotationMatrix(Angle, V4(0,0,1,0));
  m4 BillboardRotation = CoordinateSystemTransform(Direction,-CrossProduct(Right, Forward));
  m4 ModelMat = M4Identity();

  r32 Top = 1.161060;
  r32 Bot = 0.580535;
  r32 Middle = (Top + Bot) / 2.f;
  Translate(V4(0,Bot-Middle-Sqrt(3)/2.f,0,0), ModelMat);
  ModelMat = GetScaleMatrix(V4(Width,Length,1,1)) * ModelMat;
  ModelMat = StaticAngle*ModelMat;
  ModelMat = BillboardRotation*ModelMat;
  Translate(V4(Position,1),ModelMat);
  Result.ModelMat = ModelMat;
  Result.Color = Color;
  Result.Radius = 2.f;
  Result.FaceDist = 1.f;
  Result.Center = Position;
  return Result;
}

void CastConeRays(application_render_commands* RenderCommands, jwin::device_input* Input, camera* Camera, v3 PointOnUitSphere, r32 AngleOnSphere, r32 MaxAngleOnSphere, v4 Translation, m4 RotationMatrix)
{
  ray_cast* Ray = PushStruct(GlobalTransientArena, ray_cast);
  m4 StaticAngle = QuaternionAsMatrix(GetRotation(V3(0,-1,0), PointOnUitSphere));

  m4 ModelMat = M4Identity();
  Translate(V4(0,-1,0,0), ModelMat);
  ModelMat = GetScaleMatrix(V4(Sin(AngleOnSphere)*1.01,1,Sin(AngleOnSphere+0.01)*1.01,1)) * ModelMat;
  m4 TransMat = GetTranslationMatrix(Translation);
  ModelMat =  TransMat * RotationMatrix * StaticAngle * ModelMat;
  Ray->ModelMat = ModelMat;
  Ray->Color = V4(1, 1, 1, LinearRemap(MaxAngleOnSphere-AngleOnSphere, 0, MaxAngleOnSphere, 0.5,1));
  Ray->Radius = 2.f;
  Ray->FaceDist = 1.f;
  Ray->Center = V3(Translation);

  render_group* RenderGroup = RenderCommands->RenderGroup;
  render_object* RayObj = PushNewRenderObject(RenderGroup);
  RayObj->ProgramHandle = GlobalState->PlaneStarProgram;
  RayObj->MeshHandle = GlobalState->Cone;
  RayObj->FrameBufferHandle = GlobalState->TransparentFrameBuffer;

  PushUniform(RayObj, GetUniformHandle(RenderGroup, GlobalState->PlaneStarProgram, "ProjectionMat"), Camera->P);
  PushUniform(RayObj, GetUniformHandle(RenderGroup, GlobalState->PlaneStarProgram, "ViewMat"), Camera->V);

  PushInstanceData(RayObj, 1, sizeof(ray_cast), (void*) Ray);
}

void CastRays(application_render_commands* RenderCommands, jwin::device_input* Input, camera* Camera, v3 Position)
{
  r32 ThinRayCount = 15;
  r32 ThickRayCount = 7;
  ray_cast* Rays = PushArray(GlobalTransientArena, ThinRayCount + ThickRayCount, ray_cast);
  for (r32 i = 0; i < ThinRayCount; ++i)
  {
    r32 RayAngle = 0.06f*Input->Time + i*Tau32 / ThinRayCount;
    //Rays[(u32)i] = CastRay(Camera, RayAngle, 0.3, 4, V4(1-i/ThinRayCount, i/ThinRayCount, 0.81, 0.6));
    Rays[(u32)i] = CastRay(Camera, RayAngle, 0.3, 4, V4(1, 1, 1, 0.5),Position);
  }
  for (r32 i = 0; i < ThickRayCount; ++i)
  {
    r32 RayAngle = -0.04*Input->Time + i*Tau32/ThickRayCount + Pi32/3.f + 0.03*Sin(Input->Time);
    //Rays[(u32)(i + ThinRayCount)] = CastRay(Camera, RayAngle, 1, 4, V4(i/ThickRayCount, 1-i/ThickRayCount, 0.81, 0.6));
    Rays[(u32)(i + ThinRayCount)] = CastRay(Camera, RayAngle, 1, 4, V4(1, 1, 1, 0.5),Position);
  }

  render_object* Ray = PushNewRenderObject(RenderCommands->RenderGroup);
  Ray->ProgramHandle = GlobalState->PlaneStarProgram;
  Ray->MeshHandle = GlobalState->Triangle;
  Ray->FrameBufferHandle = GlobalState->TransparentFrameBuffer;

  PushUniform(Ray, GetUniformHandle(RenderCommands->RenderGroup, GlobalState->PlaneStarProgram, "ProjectionMat"), Camera->P);
  PushUniform(Ray, GetUniformHandle(RenderCommands->RenderGroup, GlobalState->PlaneStarProgram, "ViewMat"), Camera->V);
  u32 InstanceCount = ThinRayCount + ThickRayCount;
  PushInstanceData(Ray, InstanceCount, InstanceCount * sizeof(ray_cast), (void*) Rays);
}


v4 LerpColor(r32 t, v4 StartColor, v4 EndColor)
{
  v4 Result = t * (EndColor - StartColor) + StartColor;
  return Result;
}

struct eruption_params {
  r32 EruptionSize;
  v3 PointOnUnitSphere;
  r32 PopTime;
  u32 MaxEruptionBandCount;
  r32 RadiiIncrements[4];
  v4 Colors[4];
  b32 HasRayCone;
  r32 Duration;
  r32 Time;
  u32 RegionIndex;
};

void DrawEruptionBands(application_render_commands* RenderCommands, jwin::device_input* Input, u32 ParamCounts, eruption_params* Params, m4 StarModelMat, v4 Translation, m4 RotationMatrix) {

  u32 MaxBandCount = 0;
  for (int ParamIndex = 0; ParamIndex < ParamCounts; ++ParamIndex)
  {
    MaxBandCount+=Params[ParamIndex].MaxEruptionBandCount;
  }

  eurption_band* EruptionBands = PushArray(GlobalTransientArena, MaxBandCount, eurption_band);
  u32 BandCount = 0;
  for (int ParamIndex = 0; ParamIndex < ParamCounts; ++ParamIndex)
  {
    eruption_params* Param = Params + ParamIndex;
    r32 EruptionSize = Param->PopTime * Param->EruptionSize;
    r32 TimeParameter = Unlerp(Param->Time, 0, Param->Duration);
    r32 OuterBandRadii = Lerp(TimeParameter, 0, EruptionSize);
    r32 InnerBandRadii = BranchlessArithmatic( TimeParameter < Param->PopTime,
      0,
      LinearRemap(TimeParameter, Param->PopTime, 1, 0, EruptionSize));

    u32 ActiveBandCount = 0;
    r32 IncrementSum = 0;
    u32 Index = 0;
    do
    {
      ActiveBandCount++;
      IncrementSum+=Param->RadiiIncrements[Index++] * Param->PopTime;
    }while(ActiveBandCount<Param->MaxEruptionBandCount && TimeParameter > IncrementSum);

    r32 Radii = OuterBandRadii;
    for (int BandIndex = 0; BandIndex < ActiveBandCount; ++BandIndex)
    {
      eurption_band* EruptionBand = EruptionBands + BandIndex + BandCount;
      EruptionBand->Color = Param->Colors[BandIndex];
      EruptionBand->Center = Param->PointOnUnitSphere;
      EruptionBand->OuterRadii = Radii;
      EruptionBand->InnerRadii = BranchlessArithmatic(TimeParameter < Param->PopTime,
        Maximum(0, Radii - Param->RadiiIncrements[BandIndex] * EruptionSize*Param->PopTime),
        Radii - (OuterBandRadii-InnerBandRadii) * Param->RadiiIncrements[BandIndex]);
      Radii = EruptionBand->InnerRadii;
      EruptionBand->Color.W = Minimum(1, LinearRemap(Param->Time, Param->Duration-0.3f, Param->Duration, 1,0));
    }
    BandCount += ActiveBandCount;
    if(Param->HasRayCone && ActiveBandCount == Param->MaxEruptionBandCount && EruptionBands[BandCount-1].InnerRadii > 0)
    {
      CastConeRays(RenderCommands, Input, &GlobalState->Camera, Param->PointOnUnitSphere,
        2*EruptionBands[BandCount-1].InnerRadii, EruptionSize, Translation, RotationMatrix);
    }
  }

  render_object* Eruptions = PushNewRenderObject(RenderCommands->RenderGroup);
  Eruptions->ProgramHandle = GlobalState->EruptionBandProgram;
  Eruptions->MeshHandle = GlobalState->Sphere;
  Eruptions->FrameBufferHandle = GlobalState->MsaaFrameBuffer;
  PushUniform(Eruptions, GetUniformHandle(RenderCommands->RenderGroup, GlobalState->EruptionBandProgram, "ProjectionMat"), GlobalState->Camera.P);
  PushUniform(Eruptions, GetUniformHandle(RenderCommands->RenderGroup, GlobalState->EruptionBandProgram, "ModelView"), GlobalState->Camera.V*StarModelMat);
  PushInstanceData(Eruptions, BandCount, BandCount * sizeof(eurption_band), (void*) EruptionBands);
}

void InitializeEruption(eruption_params* Param, random_generator* Generator, u32 BandCount, v4* Colors, r32 MinEruptionSize, r32 MaxEruptionSize, r32 MaxRayProbability, r32 Theta, r32 Phi)
{
  Param->EruptionSize             = GetRandomReal(Generator, MinEruptionSize, MaxEruptionSize);
  Param->Duration                 = LinearRemap(Param->EruptionSize, MinEruptionSize, MaxEruptionSize, 10, 30);
  Param->Time                     = 0;
  Param->PointOnUnitSphere        = V3(Cos(Theta)*Sin(Phi), Sin(Theta)*Sin(Phi), Cos(Phi));
  Param->PopTime                  = LinearRemap(Param->EruptionSize, MinEruptionSize, MaxEruptionSize, 0.7, 0.5);
  Param->MaxEruptionBandCount     = BandCount;
  Param->HasRayCone               = GetRandomRealNorm(Generator) < LinearRemap(Param->EruptionSize, MinEruptionSize, MaxEruptionSize, 0,MaxRayProbability);

  Param->RadiiIncrements[0] = GetRandomRealNorm(Generator);
  r32 TotSum = Param->RadiiIncrements[0];
  for (int i = 1; i < Param->MaxEruptionBandCount-1; ++i)
  {
    Param->RadiiIncrements[i] = Param->RadiiIncrements[i-1] + GetRandomRealNorm(Generator);
    TotSum += Param->RadiiIncrements[i];
  }
  for (int i = 0; i < Param->MaxEruptionBandCount-1; ++i)
  {
    Param->RadiiIncrements[i] /= TotSum;
  }
  Param->RadiiIncrements[Param->MaxEruptionBandCount-1] = LinearRemap(Param->EruptionSize, MinEruptionSize, MaxEruptionSize, 1, 3);
  for (int i = 0; i < Param->MaxEruptionBandCount; ++i)
  {
    Param->Colors[i] = Colors[i];
  }
}

void RenderStar(application_state* GameState, application_render_commands* RenderCommands, jwin::device_input* Input, v3 Position)
{
  r32 StarSize = 1;
  camera* Camera = &GameState->Camera;
  render_group* RenderGroup = RenderCommands->RenderGroup;
  m4 Sphere1ModelMat = {};
  m4 Sphere1RotationMatrix = GetRotationMatrix(Input->Time/20.f, V4(0,1,0,0));
  {
    render_object* Sphere1 = PushNewRenderObject(RenderGroup);
    Sphere1->ProgramHandle = GameState->SolidColorProgram;
    Sphere1->MeshHandle = GameState->Sphere;
    Sphere1->FrameBufferHandle = GlobalState->MsaaFrameBuffer;
    r32 FinalSizeOscillation = StarSize * ( 1 + 0.01* Sin(0.05*Input->Time));
    Sphere1ModelMat = GetTranslationMatrix(V4(Position, 1))*  Sphere1RotationMatrix * GetScaleMatrix(V4(FinalSizeOscillation,FinalSizeOscillation,FinalSizeOscillation,1));

    PushUniform(Sphere1, GetUniformHandle(RenderGroup, GameState->SolidColorProgram, "ProjectionMat"), Camera->P);
    PushUniform(Sphere1, GetUniformHandle(RenderGroup, GameState->SolidColorProgram, "ModelView"), Camera->V*Sphere1ModelMat);
    PushUniform(Sphere1, GetUniformHandle(RenderGroup, GameState->SolidColorProgram, "Color"), V4(45.0/255.0, 51.0/255, 197.0/255.0, 1));
  }

  // Second Largest Sphere
  {
    render_object* Sphere2 = PushNewRenderObject(RenderGroup);
    Sphere2->ProgramHandle = GameState->SolidColorProgram;
    Sphere2->MeshHandle = GameState->Sphere;
    Sphere2->FrameBufferHandle = GlobalState->MsaaFrameBuffer;
    r32 LargeSize = 0.95 * StarSize;
    r32 LargeSizeOscillation = LargeSize * ( 1 + 0.02* Sin(0.1 * Input->Time+ 1.1));
    m4 Sphere2ModelMat = GetTranslationMatrix(V4(Position, 1)) * GetScaleMatrix(V4(LargeSizeOscillation,LargeSizeOscillation,LargeSizeOscillation,1));

    PushUniform(Sphere2, GetUniformHandle(RenderGroup, GameState->SolidColorProgram, "ProjectionMat"), Camera->P);
    PushUniform(Sphere2, GetUniformHandle(RenderGroup, GameState->SolidColorProgram, "ModelView"), Camera->V*Sphere2ModelMat);
    PushUniform(Sphere2, GetUniformHandle(RenderGroup, GameState->SolidColorProgram, "Color"), V4(56.0/255.0, 75.0/255, 220.0/255.0, 1));
  }

  {
    render_object* Sphere3 = PushNewRenderObject(RenderGroup);
    Sphere3->ProgramHandle = GameState->SolidColorProgram;
    Sphere3->MeshHandle = GameState->Sphere;
    Sphere3->FrameBufferHandle = GlobalState->MsaaFrameBuffer;
    r32 MediumSize = 0.85 * StarSize;
    r32 MediumScaleOccilation = MediumSize * ( 1 + 0.02* Sin(Input->Time+Pi32/4.f));
    m4 Sphere3ModelMat = GetTranslationMatrix(V4(Position, 1)) * GetScaleMatrix(V4(MediumScaleOccilation,MediumScaleOccilation,MediumScaleOccilation,1));

    PushUniform(Sphere3, GetUniformHandle(RenderGroup, GameState->SolidColorProgram, "ProjectionMat"), Camera->P);
    PushUniform(Sphere3, GetUniformHandle(RenderGroup, GameState->SolidColorProgram, "ModelView"), Camera->V*Sphere3ModelMat);
    PushUniform(Sphere3, GetUniformHandle(RenderGroup, GameState->SolidColorProgram, "Color"), V4(57.0/255.0, 110.0/255, 247.0/255.0, 1));
  }

  // Smallest Sphere
  {
    render_object* Sphere4 = PushNewRenderObject(RenderGroup);
    Sphere4->ProgramHandle = GameState->SolidColorProgram;
    Sphere4->MeshHandle = GameState->Sphere;
    Sphere4->FrameBufferHandle = GlobalState->MsaaFrameBuffer;
    r32 SmallSize = 0.65;
    r32 SmallScaleOccilation = SmallSize * ( 1 + 0.03* Sin(Input->Time+3/4.f *Pi32));
    m4 Sphere4ModelMat = GetTranslationMatrix(V4(Position, 1)) * GetScaleMatrix(V4(SmallScaleOccilation,SmallScaleOccilation,SmallScaleOccilation,1));

    PushUniform(Sphere4, GetUniformHandle(RenderGroup, GameState->SolidColorProgram, "ProjectionMat"), Camera->P);
    PushUniform(Sphere4, GetUniformHandle(RenderGroup, GameState->SolidColorProgram, "ModelView"), Camera->V*Sphere4ModelMat);
    PushUniform(Sphere4, GetUniformHandle(RenderGroup, GameState->SolidColorProgram, "Color"), V4(107.0/255.0, 196.0/255, 1, 1));
  }


  local_persist b32 RandomInit = false;
  local_persist eruption_params Params[SPOTCOUNT] = {};
  local_persist v4 Colors1[] = {
    V4(153.0/255.0, 173.0/255, 254.0/255.0, 1),
    V4(191.0/255.0, 238.0/255, 254.0/255.0, 1),
    V4(254.0/255.0, 254.0/255.0, 255/255, 1)
  };
  local_persist v4 Colors2[] = {
    V4(79/255.f, 187/255.f, 255/255.f, 1),
    V4(50/255.f, 156/255.f, 185/255.f, 1),
    V4(48.f/255.f, 51/255.f, 211/255.f, 1)
  };

  local_persist r32 FillRates[8] = {};
  local_persist r32 AngleSpans[8][4]
  {
    // Min Theta, Min Phi, Max Theta, Max Phi
    {0 * Tau32/4.f,        0, 1 * Tau32/4.f, Pi32/2.f},
    {1 * Tau32/4.f,        0, 2 * Tau32/4.f, Pi32/2.f},
    {2 * Tau32/4.f,        0, 3 * Tau32/4.f, Pi32/2.f},
    {3 * Tau32/4.f,        0, 4 * Tau32/4.f, Pi32/2.f},
    {0 * Tau32/4.f, Pi32/2.f, 1 * Tau32/4.f, Pi32},
    {1 * Tau32/4.f, Pi32/2.f, 2 * Tau32/4.f, Pi32},
    {2 * Tau32/4.f, Pi32/2.f, 3 * Tau32/4.f, Pi32},
    {3 * Tau32/4.f, Pi32/2.f, 4 * Tau32/4.f, Pi32}
  };

  if(!RandomInit)
  {
    for (int i = 0; i < SPOTCOUNT; ++i)
    {
      eruption_params* Param = Params + i;

      u32 EmptiestRegionIndex = 0;
      r32 EmptiestRegionFillRate = R32Max;
      for (int i = 0; i < ArrayCount(FillRates); ++i)
      {
        if(FillRates[i] < EmptiestRegionFillRate)
        {
          EmptiestRegionFillRate = FillRates[i];
          EmptiestRegionIndex = i;
        }
      }

      r32 Theta = GetRandomReal(&GameState->RandomGenerator, AngleSpans[EmptiestRegionIndex][0], AngleSpans[EmptiestRegionIndex][2]);
      r32 Phi   = GetRandomReal(&GameState->RandomGenerator, AngleSpans[EmptiestRegionIndex][1], AngleSpans[EmptiestRegionIndex][3]);
      if(GetRandomRealNorm(&GameState->RandomGenerator) < 0.7) {
        InitializeEruption(Param, &GameState->RandomGenerator, ArrayCount(Colors1), Colors1,0.1, 0.275, 0.7, Theta, Phi);
      }else{
        InitializeEruption(Params + i, &GameState->RandomGenerator, ArrayCount(Colors2), Colors2, 0.05, 0.14, 0, Theta, Phi);
      }
      Param->Time     = GetRandomReal(&GameState->RandomGenerator, 0, Param->Duration);
      FillRates[EmptiestRegionIndex] += Param->EruptionSize;
      Param->RegionIndex = EmptiestRegionIndex;
    }
    RandomInit = true;
  }
  
  for (int i = 0; i < SPOTCOUNT; ++i)
  {
    eruption_params* Param = Params + i;
    Param->Time += Input->deltaTime;
    if(Param->Time > Param->Duration)
    {
      u32 EmptiestRegionIndex = 0;
      r32 EmptiestRegionFillRate = R32Max;
      FillRates[Param->RegionIndex] -= Param->EruptionSize;
      for (int i = 0; i < ArrayCount(FillRates); ++i)
      {
        if(FillRates[i] < EmptiestRegionFillRate)
        {
          EmptiestRegionFillRate = FillRates[i];
          EmptiestRegionIndex = i;
        }
      }
      r32 Theta = GetRandomReal(&GameState->RandomGenerator, AngleSpans[EmptiestRegionIndex][0], AngleSpans[EmptiestRegionIndex][2]);
      r32 Phi   = GetRandomReal(&GameState->RandomGenerator, AngleSpans[EmptiestRegionIndex][1], AngleSpans[EmptiestRegionIndex][3]);
      eruption_params* Param = Params + i;
      if(GetRandomRealNorm(&GameState->RandomGenerator) < 0.7)
      {
        InitializeEruption(Param, &GameState->RandomGenerator, ArrayCount(Colors1), Colors1,  0.1, 0.275, 0.7, Theta, Phi);
      }else{
        InitializeEruption(Param, &GameState->RandomGenerator, ArrayCount(Colors2), Colors2, 0.05, 0.14, 0, Theta, Phi);
      }
      FillRates[EmptiestRegionIndex] += Param->EruptionSize;
      Param->RegionIndex = EmptiestRegionIndex;
    }
  }

  DrawEruptionBands(RenderCommands, Input, SPOTCOUNT, Params, Sphere1ModelMat, V4(Position,1), Sphere1RotationMatrix);
  
  // Ray
  CastRays(RenderCommands, Input, Camera, Position);

  // Halo
  {
    v3 Forward, Up, Right;
    GetCameraDirections(Camera, &Up, &Right, &Forward);
    v3 Direction = GetCameraPosition(Camera) - Position;
    m4 BillboardRotation = CoordinateSystemTransform(Direction,-CrossProduct(Right, Forward));
    m4 HaloModelMat = M4Identity();
    HaloModelMat = GetRotationMatrix(Pi32/2.f, V4(1,0,0,0))* HaloModelMat;
    HaloModelMat = GetScaleMatrix(V4(2,2,2,1)) * HaloModelMat;
    HaloModelMat = GetTranslationMatrix(V4(Position,0)) * BillboardRotation*HaloModelMat;

    render_object* Halo = PushNewRenderObject(RenderCommands->RenderGroup);
    Halo->ProgramHandle = GameState->PlaneStarProgram;
    Halo->MeshHandle = GameState->Plane;
    Halo->FrameBufferHandle = GlobalState->TransparentFrameBuffer;

    PushUniform(Halo, GetUniformHandle(RenderCommands->RenderGroup, GameState->PlaneStarProgram, "ProjectionMat"), Camera->P);
    PushUniform(Halo, GetUniformHandle(RenderCommands->RenderGroup, GameState->PlaneStarProgram, "ViewMat"), Camera->V);
    ray_cast* HaloRay = PushStruct(GlobalTransientArena, ray_cast);
    HaloRay->ModelMat = HaloModelMat;
    HaloRay->Color = V4(254.0/255.0, 254.0/255.0, 255/255, 0.3);
    HaloRay->Radius = (r32)( 1.3f + 0.07 * Sin(Input->Time));
    HaloRay->FaceDist =  0.3f;
    HaloRay->Center = Position;
    PushInstanceData(Halo, 1, sizeof(ray_cast), (void*) HaloRay);
  }
}

u32 CreateColoredSquareOverlayProgram(render_group* RenderGroup)
{
  u32 ProgramHandle = NewShaderProgram(RenderGroup, "ColoredOverlayQuad");
  AddUniform(RenderGroup, UniformType::M4,  ProgramHandle, "Projection");
  AddVarying(RenderGroup, UniformType::V4,  ProgramHandle, "InColor");
  AddVarying(RenderGroup, UniformType::M4,  ProgramHandle, "Model");
  
  CompileShader(RenderGroup, ProgramHandle, 
    1, LoadFileFromDisk("..\\jwin\\shaders\\ColoredOverlayQuadVertex.glsl"),
    1, LoadFileFromDisk("..\\jwin\\shaders\\ColoredOverlayQuadFragment.glsl"));
  return ProgramHandle;
}

u32 CreateTexturedSquareOverlayProgram(render_group* RenderGroup)
{
  u32 ProgramHandle = NewShaderProgram(RenderGroup, "TexturedOverlayQuad");
  AddUniform(RenderGroup, UniformType::M4,  ProgramHandle, "Projection");
  AddUniform(RenderGroup, UniformType::U32, ProgramHandle, "RenderedTexture");
  AddVarying(RenderGroup, UniformType::V4,  ProgramHandle, "Color");
  AddVarying(RenderGroup, UniformType::V4,  ProgramHandle, "Texture");
  AddVarying(RenderGroup, UniformType::M4,  ProgramHandle, "Model");
  
  CompileShader(RenderGroup, ProgramHandle, 
    1, LoadFileFromDisk("..\\jwin\\shaders\\TexturedOverlayQuadVertex.glsl"),
    1, LoadFileFromDisk("..\\jwin\\shaders\\TexturedOverlayQuadFragment.glsl"));
  return ProgramHandle;
}

u32 CreateFontProgram(render_group* RenderGroup)
{
  u32 ProgramHandle = NewShaderProgram(RenderGroup, "FontRenderProgram");

  AddUniform(RenderGroup, UniformType::M4,  ProgramHandle, "Projection");
  AddUniform(RenderGroup, UniformType::U32, ProgramHandle, "RenderedTexture");
  AddUniform(RenderGroup, UniformType::R32, ProgramHandle, "OnEdgeValue");
  AddUniform(RenderGroup, UniformType::R32, ProgramHandle, "PixelDistanceScale");
  AddVarying(RenderGroup, UniformType::V4,  ProgramHandle, "TextColor_in");
  AddVarying(RenderGroup, UniformType::V4,  ProgramHandle, "TexCoord_in");
  AddVarying(RenderGroup, UniformType::M4,  ProgramHandle, "Model");
  CompileShader(RenderGroup, ProgramHandle, 
     1, LoadFileFromDisk("..\\jwin\\shaders\\FontRenderVertex.glsl"),
     1, LoadFileFromDisk("..\\jwin\\shaders\\FontRenderFragment.glsl"));
  return ProgramHandle;
}


u32 CreateGaussianBlurProgramY(render_group* RenderGroup)
{
  u32 ProgramHandleY = NewShaderProgram(RenderGroup, "GaussoanYProgram");

  AddUniform(RenderGroup, UniformType::U32, ProgramHandleY, "RenderedTexture");
  AddUniform(RenderGroup, UniformType::V2,  ProgramHandleY, "sideSize");
  AddUniform(RenderGroup, UniformType::R32, ProgramHandleY, "offset");
  AddUniform(RenderGroup, UniformType::R32, ProgramHandleY, "weight");
  AddUniform(RenderGroup, UniformType::U32, ProgramHandleY, "kernerlSize");
  CompileShader(RenderGroup, ProgramHandleY,
    1, LoadFileFromDisk("..\\jwin\\shaders\\gaussian_vertex_y.glsl"),
    1, LoadFileFromDisk("..\\jwin\\shaders\\gaussian_fragment_y.glsl"));
  return ProgramHandleY;
}

u32 CreateGaussianBlurProgramX(render_group* RenderGroup)
{
  u32 ProgramHandleX = NewShaderProgram(RenderGroup,"GaussoanXProgram");

  AddUniform(RenderGroup, UniformType::U32, ProgramHandleX, "RenderedTexture");
  AddUniform(RenderGroup, UniformType::V2,  ProgramHandleX, "sideSize");
  AddUniform(RenderGroup, UniformType::R32, ProgramHandleX, "offset");
  AddUniform(RenderGroup, UniformType::R32, ProgramHandleX, "weight");
  AddUniform(RenderGroup, UniformType::U32, ProgramHandleX, "kernerlSize");
  CompileShader(RenderGroup, ProgramHandleX,
    1, LoadFileFromDisk("..\\jwin\\shaders\\gaussian_vertex_x.glsl"),
    1, LoadFileFromDisk("..\\jwin\\shaders\\gaussian_fragment_x.glsl"));
  return ProgramHandleX;
}

u32 CreateTransparentCompositionProgram(render_group* RenderGroup)
{

  local_persist char* TransparentCompositionVertexShaderCodeArr[1] = {};
  local_persist char* TransparentCompositionFragmentShaderCodeArr[1]  = {};


  u32 ProgramHandle = NewShaderProgram(RenderGroup,
    "TransparentCompositionProgram");

  AddUniform(RenderGroup, UniformType::U32, ProgramHandle, "AccumTex");
  AddUniform(RenderGroup, UniformType::U32, ProgramHandle, "RevealTex");
  CompileShader(RenderGroup, ProgramHandle,  
    1, LoadFileFromDisk("..\\jwin\\shaders\\transparent_composition_vertex.glsl"),
    1, LoadFileFromDisk("..\\jwin\\shaders\\transparent_composition_fragment.glsl"));
  return ProgramHandle;
}

u32 PushBlitPlaneMesh(render_group* RenderGroup)
{
  u32 PlaneIndex[] = {
    0,1,2,
    2,1,3
  };
  opengl_vertex PlaneVertex [] = {
    {{-1.0f, -1.0f, 0.0f},{0,0,0},{0,0}},
    {{ 1.0f, -1.0f, 0.0f},{0,0,0},{1,0}},
    {{-1.0f,  1.0f, 0.0f},{0,0,0},{0,1}},
    {{ 1.0f,  1.0f, 0.0f},{0,0,0},{1,1}},
  };

  gl_vertex_buffer* VertexBuffer = PushStruct(GlobalTransientArena, gl_vertex_buffer);
  VertexBuffer->IndexCount = ArrayCount(PlaneIndex);
  VertexBuffer->Indeces = (u32*) PushCopy(GlobalTransientArena, sizeof(PlaneIndex), PlaneIndex);
  VertexBuffer->VertexCount = ArrayCount(PlaneVertex);
  VertexBuffer->VertexData = (opengl_vertex*) PushCopy(GlobalTransientArena, sizeof(PlaneVertex), PlaneVertex);
  opengl_buffer_data GlBufferData = {};
  GlBufferData.BufferCount = 1;
  GlBufferData.BufferData = VertexBuffer;
  u32 Result = PushNewMesh(RenderGroup, GlBufferData);
  return Result;
}

world InitiateWorld(application_render_commands* RenderCommands)
{
  world Result = {};
  Result.EntityManager = ecs::CreateEntityManager();
  Result.RenderSystem = ecs::render::CreateRenderSystem(RenderCommands->RenderGroup, RenderCommands->WindowInfo.Width, RenderCommands->WindowInfo.Height, RenderCommands);

  return Result;
}

MENU_DRAW(RenderScene)
{
  ecs::render::window_size_pixel* Window = &GetRenderSystem()->WindowSize;
  r32 PixelSize = 1/Window->WindowWidth;
  rect2f Region = Node->Region;
  Region.X += PixelSize;
  Region.Y += PixelSize;
  ecs::render::SetDrawWindowCanCord(GetRenderSystem(), Region);
  ecs::render::DrawScene(GetRenderSystem(), GetEntityManager());
}
//TODO: Super hacky, do better
//      Maybe make it so that the update function _only_ gets called when the container_node it's attached to has focus
b32 IsSceneSelected()
{
  b32 Result = (IsNodeSelected(GlobalState->World.MenuInterface, GlobalState->World.ScenePlugin) && 
                IsFocusWindow(GlobalState->World.MenuInterface, GetMenu(GlobalState->World.MenuInterface, GlobalState->World.ScenePlugin)));
  return Result;
}

void SceneInput(camera* Camera, jwin::device_input* Input)
{
  { // Keyboard
    local_persist v3 LightPosition = V3(0,3,0);
    local_persist r32 near = 0.001;

    if((jwin::Active(Input->Keyboard.Key_LSHIFT) || jwin::Active(Input->Keyboard.Key_RSHIFT)))
    {
      if(Pushed(Input->Keyboard.Key_UP))
      {
        m4 V = Camera->V;
        r32 AngleOfView = Camera->AngleOfView;
        r32 AspectRatio = Camera->AspectRatio;

        InitiateCamera(Camera, AngleOfView+1, AspectRatio, near);
        Camera->V = V;
        Platform.DEBUGPrint("AngleOfView: %f\n", AspectRatio*Camera->AngleOfView);
      }else if(Pushed(Input->Keyboard.Key_DOWN))
      {
        m4 V = Camera->V;
        r32 AngleOfView = Camera->AngleOfView;
        r32 AspectRatio = Camera->AspectRatio;

        InitiateCamera(Camera, AngleOfView-1, AspectRatio, near);
        Camera->V = V;

        Platform.DEBUGPrint("AngleOfView: %f\n", AspectRatio*Camera->AngleOfView);
      }
    }else{
      if(Pushed(Input->Keyboard.Key_UP))
      {
        m4 V = Camera->V;
        r32 AngleOfView = Camera->AngleOfView;
        r32 AspectRatio = Camera->AspectRatio;
        near = near*1.1;
        InitiateCamera(Camera, AngleOfView, AspectRatio, near);
        Camera->V = V;
        Platform.DEBUGPrint("Near: %f\n", near);
      }else if(Pushed(Input->Keyboard.Key_DOWN))
      {
        m4 V = Camera->V;
        r32 AngleOfView = Camera->AngleOfView;
        r32 AspectRatio = Camera->AspectRatio;
        near = near * 0.9;
        InitiateCamera(Camera, AngleOfView, AspectRatio, near);
        Camera->V = V;

        Platform.DEBUGPrint("Near: %f\n", near);
      }
    }

    r32 Len = 0;
    v3 Pos = V3(Len,0,0);
    v3 At = V3(0,0,0);
    v3 Up = V3(0,1,0);
    b32 UpdateCamera = false;
    if(!(jwin::Active(Input->Keyboard.Key_LALT) || jwin::Active(Input->Keyboard.Key_RALT)))
    {
      if(jwin::Pushed(Input->Keyboard.Key_X))
      {
        UpdateCamera = true;
        Pos = V3(Len,0,0);
        At = V3(Len+1,0,0);
        if((jwin::Active(Input->Keyboard.Key_LSHIFT) || jwin::Active(Input->Keyboard.Key_RSHIFT))) 
        {
          Pos = -Pos;
          At = At = V3(Len-1,0,0);
        }
      }
      else if(jwin::Pushed(Input->Keyboard.Key_Y))
      {
        UpdateCamera = true;
        Pos = V3(0,Len,0);
        At = V3(0,Len + 1,0);
        Up = V3(1,0,0);
        if((jwin::Active(Input->Keyboard.Key_LSHIFT) || jwin::Active(Input->Keyboard.Key_RSHIFT)))
        {
          Pos = -Pos;
          At = V3(0,Len - 1,0);
        }
      }
      else if(jwin::Pushed(Input->Keyboard.Key_Z))
      {
        UpdateCamera = true;
        Pos = V3(0,0,Len);
        At = V3(0,0,Len + 1);
        if((jwin::Active(Input->Keyboard.Key_LSHIFT) || jwin::Active(Input->Keyboard.Key_RSHIFT)))
        {
          Pos = -Pos;
          At = V3(0,0,Len - 1);
        }  
      }
      else if(jwin::Pushed(Input->Keyboard.Key_Q))
      {
        UpdateCamera = true;
        Pos = V3(0,0,4);
        if((jwin::Active(Input->Keyboard.Key_LSHIFT) || jwin::Active(Input->Keyboard.Key_RSHIFT)))
        {
          Pos = -Pos;
          At = V3(0,0,0);
        }
      }

      if(UpdateCamera)
      {
        LookAt(Camera, Pos, At, Up);
        v3 Up, Right, Forward;
        GetCameraDirections(Camera, &Up, &Right, &Forward);
        v3 CamPos = GetCameraPosition(Camera);
      }
    }else{
      if(jwin::Pushed(Input->Keyboard.Key_X))
      {
        Pos = V3(Len,0,0);
        if(!(jwin::Active(Input->Keyboard.Key_LSHIFT) || jwin::Active(Input->Keyboard.Key_RSHIFT)))
        {
          LightPosition = Pos;
        }else{
          LightPosition = -Pos;
        }
        Platform.DEBUGPrint("LightPos: %f %f %f\n", LightPosition.X, LightPosition.Y, LightPosition.Z);
      }else if(jwin::Pushed(Input->Keyboard.Key_Y)){
        Pos = V3(0,Len,0);
        if(!(jwin::Active(Input->Keyboard.Key_LSHIFT) || jwin::Active(Input->Keyboard.Key_RSHIFT)))
        {
          LightPosition = Pos;
        }else{
          LightPosition = -Pos;
        }
        Platform.DEBUGPrint("LightPos: %f %f %f\n", LightPosition.X, LightPosition.Y, LightPosition.Z);
      }else if(jwin::Pushed(Input->Keyboard.Key_Z)){
        Pos = V3(0,0,Len);
        if(!(jwin::Active(Input->Keyboard.Key_LSHIFT) || jwin::Active(Input->Keyboard.Key_RSHIFT)))
        {
          LightPosition = Pos;
        }else{
          LightPosition = -Pos;
        }
        Platform.DEBUGPrint("LightPos: %f %f %f\n", LightPosition.X, LightPosition.Y, LightPosition.Z);
      }else if(jwin::Pushed(Input->Keyboard.Key_Q)){
        Pos = V3(0,Len,Len);
        if(!(jwin::Active(Input->Keyboard.Key_LSHIFT) || jwin::Active(Input->Keyboard.Key_RSHIFT)))
        {
          LightPosition = Pos;
        }else{
          LightPosition = -Pos;
        }
        Platform.DEBUGPrint("LightPos: %f %f %f\n", LightPosition.X, LightPosition.Y, LightPosition.Z);
      }
    }
    
    r32 CamSpeed = 0.05;
    if(jwin::Active(Input->Keyboard.Key_C))
    {
      SetCameraPosition(Camera, V3(0,0,0));
    }
    if(jwin::Active(Input->Keyboard.Key_W))
    {
      TranslateCamera(Camera, V3(0,0,-CamSpeed));
    }
    if(jwin::Active(Input->Keyboard.Key_S))
    {
      TranslateCamera(Camera, V3(0,0,CamSpeed));
    }
    if(jwin::Active(Input->Keyboard.Key_A))
    {
      TranslateCamera(Camera, V3(-CamSpeed,0,0));
    }
    if(jwin::Active(Input->Keyboard.Key_D))
    {
      TranslateCamera(Camera, V3(CamSpeed,0,0));
    }
    if(jwin::Active(Input->Keyboard.Key_R))
    {
      TranslateCamera(Camera, V3(0,CamSpeed,0));
    }
    if(jwin::Active(Input->Keyboard.Key_F))
    {
      TranslateCamera(Camera, V3(0,-CamSpeed,0));
    }
  }

  { 
    v3 WUp, WRight, WForward;
    v3 Up = V3(0,1,0);
    GetCameraDirections(Camera, &WUp, &WRight, &WForward);
    if(!Input->Mouse.ShowMouse || jwin::Active(Input->Mouse.Button[jwin::MouseButton_Left]) || jwin::Active(Input->Mouse.Button[jwin::MouseButton_Middle]))
    {
      if(!jwin::Active(Input->Mouse.Button[jwin::MouseButton_Middle]))
      {
        if(Input->Mouse.dX != 0)
        {
          //RotateAround(Camera, -5*Input->Mouse.dX, Up);
          RotateCameraAroundWorldAxis(Camera, -2*Input->Mouse.dX, V3(0,1,0) );
          //RotateCamera(Camera, 2*Input->Mouse.dX, V3(0,-1,0) );
        }
        if(Input->Mouse.dY != 0)
        {
          RotateCamera(Camera, 2*Input->Mouse.dY, V3(1,0,0) );      
          v3 CamPos = GetCameraPosition(Camera);
        }
      }else{
        if(Input->Mouse.dX != 0)
        {
          RotateAround(Camera, -5*Input->Mouse.dX, WUp);
          char Buf[32] = {};
          jstr::ToString( WRight.E, 2, ArrayCount(Buf), Buf );
          Platform.DEBUGPrint("Right   : %s\n", Buf);
        }
        if(Input->Mouse.dY != 0)
        {
          RotateAround(Camera, -5*Input->Mouse.dY, -WRight);
          char Buf[32] = {};
          jstr::ToString( Up.E, 2, ArrayCount(Buf), Buf );
          Platform.DEBUGPrint("Up: %s\n", Buf);
        }
      }
    }
  }
}


MENU_UPDATE_FUNCTION(SceneTakeInput)// b32 name( menu_interface* Interface, container_node* CallerNode, void* Data )
{
  if(IsSceneSelected())
  {
    camera* Camera = &GlobalState->Camera;
    jwin::device_input* Input = (jwin::device_input*) Data;
    SceneInput(Camera, Input);
  }
  return true;
}

container_node* GetDEBUGSquareNode(menu_region_alignment XAlignment, r32 XSize, menu_region_alignment YAlignment, r32 YSize, umm ColorIndex)
{
  container_node* ColorNode = NewContainer(GetMenuInterface(), container_type::None);
  
  size_attribute* SizeAttr = (size_attribute*) PushAttribute(GetMenuInterface(), ColorNode, ATTRIBUTE_SIZE);
  SizeAttr->Width = ContainerSizeT(menu_size_type::RELATIVE_, XSize);
  SizeAttr->Height = ContainerSizeT(menu_size_type::RELATIVE_, YSize);
  SizeAttr->LeftOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0.00);
  SizeAttr->TopOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0.00);
  alignment_attribute* AlignAttr = (alignment_attribute*) PushAttribute(GetMenuInterface(), ColorNode, ATTRIBUTE_ALIGNMENT);
  AlignAttr->XAlignment = menu_region_alignment::CENTER;
  AlignAttr->YAlignment = menu_region_alignment::CENTER;

  SetColor(GetMenuInterface(), ColorNode, menu::GetColor(GetColorTable(), ColorIndex));

  return ColorNode;
}

void SetDEBUGSquareNode(container_node* Node,
                        menu_size_type XSizeType, r32 XSize, 
                        menu_size_type YSizeType, r32 YSize, 
                        menu_region_alignment XAlignment,
                        menu_region_alignment YAlignment)
{
 
  size_attribute* SizeAttr = (size_attribute*) GetAttributePointer(Node, ATTRIBUTE_SIZE);
  SizeAttr->Width = ContainerSizeT(XSizeType, XSize);
  SizeAttr->Height = ContainerSizeT(YSizeType, YSize);
  SizeAttr->LeftOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0.00);
  SizeAttr->TopOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0.00);
  alignment_attribute* AlignAttr = (alignment_attribute*) GetAttributePointer(Node, ATTRIBUTE_ALIGNMENT);
  AlignAttr->XAlignment = XAlignment;
  AlignAttr->YAlignment = YAlignment;
}





// void ApplicationUpdateAndRender(application_memory* Memory, application_render_commands* RenderCommands, jwin::device_input* Input)
extern "C" JWIN_UPDATE_AND_RENDER(ApplicationUpdateAndRender)
{
  GlobalState = JwinBeginFrameMemory(application_state);
  GlobalInput = Input;
  GlobalImguiContext = &GlobalState->ImguiContext;
  ResetRenderGroup(RenderCommands->RenderGroup);
  platform_offscreen_buffer* OffscreenBuffer = &RenderCommands->PlatformOffscreenBuffer;
  ImguiBegin(Input);
  g_t = Input->Time;

  if(!GlobalState->Initialized)
  {
    GlobalState->ColorTable = menu::CreateColorTable(GlobalPersistentArena);
    RenderCommands->RenderGroup = InitiateRenderGroup();
    GlobalState->World = InitiateWorld(RenderCommands);
    ecs::render::window_size_pixel* Window = &GlobalState->World.RenderSystem->WindowSize;
    obj_loaded_file* plane = ReadOBJFile(GlobalPersistentArena, GlobalTransientArena, "..\\data\\checker_plane_simple.obj");
    obj_loaded_file* billboard = ReadOBJFile(GlobalPersistentArena, GlobalTransientArena, "..\\data\\plane.obj");
    obj_loaded_file* sphere = ReadOBJFile(GlobalPersistentArena, GlobalTransientArena, "..\\data\\sphere.obj");
    obj_loaded_file* triangle = ReadOBJFile(GlobalPersistentArena, GlobalTransientArena, "..\\data\\triangle.obj");
    obj_loaded_file* cone = ReadOBJFile(GlobalPersistentArena, GlobalTransientArena, "..\\data\\cone.obj");
    obj_loaded_file* cube = ReadOBJFile(GlobalPersistentArena, GlobalTransientArena, "..\\data\\qube.obj");
    obj_loaded_file* cylinder = ReadOBJFile(GlobalPersistentArena, GlobalTransientArena, "..\\data\\cylinder.obj");

    render_group* RenderGroup = RenderCommands->RenderGroup;
    // This memory only needs to exist until the data is loaded to the GPU
    GlobalState->PhongProgram = CreatePhongProgram(RenderGroup);
    GlobalState->PhongProgramTransparent = CreatePhongTransparentProgram(RenderGroup);
    GlobalState->PlaneStarProgram = CreatePlaneStarProgram(RenderGroup);
    GlobalState->SolidColorProgram = CreateSolidColorProgram(RenderGroup);
    GlobalState->EruptionBandProgram = CreateEruptionBandProgram(RenderGroup);
    GlobalState->TransparentCompositionProgram = CreateTransparentCompositionProgram(RenderGroup);
    GlobalState->GaussianProgramX = CreateGaussianBlurProgramX(RenderGroup);
    GlobalState->GaussianProgramY = CreateGaussianBlurProgramY(RenderGroup);
    GlobalState->FontRenterProgram =  CreateFontProgram(RenderGroup);
    GlobalState->ColoredSquareOverlayProgram = CreateColoredSquareOverlayProgram(RenderGroup);
    GlobalState->TexturedSquareOverlayProgram = CreateTexturedSquareOverlayProgram(RenderGroup);


    GlobalState->Cube = PushNewMesh(RenderGroup, MapObjToOpenGLMesh(GlobalTransientArena, cube));
    GlobalState->Plane = PushNewMesh(RenderGroup, MapObjToOpenGLMesh(GlobalTransientArena, plane));
    GlobalState->Sphere = PushNewMesh(RenderGroup, MapObjToOpenGLMesh(GlobalTransientArena, sphere));
    GlobalState->Cone = PushNewMesh(RenderGroup, MapObjToOpenGLMesh(GlobalTransientArena, cone));
    GlobalState->Cylinder = PushNewMesh(RenderGroup, MapObjToOpenGLMesh(GlobalTransientArena, cylinder));
    GlobalState->Triangle = PushNewMesh(RenderGroup, MapObjToOpenGLMesh(GlobalTransientArena, triangle));
    GlobalState->Billboard = PushNewMesh(RenderGroup, MapObjToOpenGLMesh(GlobalTransientArena,  billboard));
    GlobalState->BlitPlane =  PushBlitPlaneMesh(RenderGroup);

    GlobalState->ImguiContext.Icons = LoadImguiIcons(RenderGroup);
    GlobalState->ApplicationImgui = CreateApplicationImgui(GlobalPersistentArena, &GlobalState->ImguiContext, GlobalState->ColorTable.ColorCount);

    obj_bitmap* BrickWallTexture = LoadTGA(GlobalTransientArena, "..\\data\\textures\\brick_wall_base.tga");
    obj_bitmap* FadedRayTexture = LoadTGA(GlobalTransientArena, "..\\data\\textures\\faded_ray.tga");
    obj_bitmap* EarthTexture = LoadTGA(GlobalTransientArena, "..\\data\\textures\\8081_earthmap4k.tga");

    GlobalState->CheckerBoardTexture = Push32BitColorTexture(RenderGroup, plane->MaterialData->Materials[0].MapKd);
    GlobalState->BrickWallTexture = Push32BitColorTexture(RenderGroup, BrickWallTexture);
    GlobalState->FadedRayTexture = Push32BitColorTexture(RenderGroup, FadedRayTexture);
    GlobalState->EarthTexture = Push32BitColorTexture(RenderGroup, EarthTexture);
    
    texture_params DefaultColor = DefaultColorTextureParams();
    texture_params DefaultDepth = DefaultDepthTextureParams();
    texture_params RevealTexParam = DefaultColorTextureParams();
    RevealTexParam.TextureFormat = texture_format::R_8;
    

    GlobalState->MsaaColorTexture = PushNewTexture(RenderGroup, Window->MSAA * Window->ApplicationWidth, Window->MSAA * Window->ApplicationHeight, DefaultColor, 0);
    GlobalState->MsaaDepthTexture = PushNewTexture(RenderGroup, Window->MSAA * Window->ApplicationWidth, Window->MSAA * Window->ApplicationHeight, DefaultDepth, 0);
    GlobalState->AccumTexture     = PushNewTexture(RenderGroup, Window->MSAA * Window->ApplicationWidth, Window->MSAA * Window->ApplicationHeight, DefaultColor, 0);
    GlobalState->RevealTexture    = PushNewTexture(RenderGroup, Window->MSAA * Window->ApplicationWidth, Window->MSAA * Window->ApplicationHeight, RevealTexParam, 0);
    GlobalState->GaussianATexture = PushNewTexture(RenderGroup, Window->ApplicationWidth, Window->ApplicationHeight, DefaultColor, 0);
    GlobalState->GaussianBTexture = PushNewTexture(RenderGroup, Window->ApplicationWidth, Window->ApplicationHeight, DefaultColor, 0);
    

    u32 TransparentColorTexture[]       = {GlobalState->AccumTexture,GlobalState->RevealTexture};
    GlobalState->DefaultFrameBuffer     = PushNewFrameBuffer(RenderGroup,  Window->ApplicationWidth,       Window->ApplicationHeight, 0, 0, 0, 0);
    GlobalState->MsaaFrameBuffer        = PushNewFrameBuffer(RenderGroup,  Window->MSAA * Window->ApplicationWidth,  Window->MSAA * Window->ApplicationHeight, 1, &GlobalState->MsaaColorTexture, GlobalState->MsaaDepthTexture, 0);
    GlobalState->TransparentFrameBuffer = PushNewFrameBuffer(RenderGroup,  Window->MSAA * Window->ApplicationWidth,  Window->MSAA * Window->ApplicationHeight, ArrayCount(TransparentColorTexture), TransparentColorTexture, GlobalState->MsaaDepthTexture, 0);
    GlobalState->GaussianAFrameBuffer   = PushNewFrameBuffer(RenderGroup,  Window->ApplicationWidth,       Window->ApplicationHeight, 1, &GlobalState->GaussianATexture, 0, 0);
    GlobalState->GaussianBFrameBuffer   = PushNewFrameBuffer(RenderGroup,  Window->ApplicationWidth,       Window->ApplicationHeight, 1, &GlobalState->GaussianBTexture, 0, 0);

    texture_params WhitePixelParam = DefaultColorTextureParams();
    WhitePixelParam.TextureFormat = texture_format::RGBA_U8;
    WhitePixelParam.InputDataType = OPEN_GL_UNSIGNED_BYTE;
    u8 WhitePixel[4] = {255,255,255,255};
    void* WhitePixelPtr = PushCopy(GlobalTransientArena, sizeof(WhitePixel), (void*) WhitePixel);
    GlobalState->WhitePixelTexture = PushNewTexture(RenderGroup, 1, 1, WhitePixelParam, WhitePixelPtr);


    GlobalState->Initialized = true;

    GlobalState->Camera = {};
    InitiateCamera(&GlobalState->Camera, 70, GlobalState->World.RenderSystem->WindowSize.ApplicationAspectRatio, 0.1);
    LookAt(&GlobalState->Camera, V3(0,0,4), V3(0,0,0));
 
    GlobalState->RandomGenerator = RandomGenerator(Input->RandomSeed);

    GlobalState->DebugRenderCommands = PushStruct(GlobalPersistentArena, debug_application_render_commands);
    *GlobalState->DebugRenderCommands = DebugApplicationRenderCommands(RenderCommands, &GlobalState->Camera);
    GlobalState->DebugRenderCommands->MsaaFrameBuffer = GlobalState->MsaaFrameBuffer;
    GlobalState->DebugRenderCommands->DefaultFrameBuffer = GlobalState->DefaultFrameBuffer;

    GlobalState->FunctionPool = PushStruct(GlobalPersistentArena, function_pool);
    
    GlobalState->World.MenuInterface = CreateMenuInterface(GlobalPersistentArena, &Input->Keyboard, Megabytes(1), GlobalState->World.RenderSystem->WindowSize.ApplicationAspectRatio);
    menu_interface* Interface = GlobalState->World.MenuInterface;
    container_node* DefaultWindow = 0;
    {
      menu_tree* WindowsDropDownMenu = CreateNewDropDownMenuItem(GlobalState->World.MenuInterface, "Windows");
      {
        // Create Scene Window
        container_node* ScenePlugin = CreatePlugin(Interface, "Scene");
        AddPlugintoMainMenu(Interface, WindowsDropDownMenu, ScenePlugin);

        container_node* SceneContainer =  NewContainer(Interface);
        SceneContainer->Functions.Draw = DeclareFunction(menu_draw, RenderScene);
        PushToUpdateQueue(Interface, SceneContainer, SceneTakeInput, (void*) Input, false);
        ConnectNodeToBack(ScenePlugin, SceneContainer);
        GlobalState->World.ScenePlugin = ScenePlugin;

        DefaultWindow = SetDefaultPlugin(Interface, ScenePlugin);
      }
      {
        container_node* EntityContainer =  NewContainer(Interface, container_type::Grid);
        grid_node* Grid = GetGridNode(EntityContainer);
        Grid->Col = 1;
        Grid->Row = 0;
        Grid->TotalMarginX = 0.0;
        Grid->TotalMarginY = 0.0;
        Grid->Stack = true;
        Grid->StackXAlignment = menu_region_alignment::LEFT;
        Grid->StackYAlignment = menu_region_alignment::TOP;

        color_attribute* BackgroundColor = (color_attribute* ) PushAttribute(Interface, EntityContainer, ATTRIBUTE_COLOR);
        BackgroundColor->Color = V4(0.2,0,0,1);

        size_attribute* SizeAttr = (size_attribute*) PushAttribute(Interface, EntityContainer, ATTRIBUTE_SIZE);
        SizeAttr->Width = ContainerSizeT(menu_size_type::RELATIVE_, 1);
        SizeAttr->Height = ContainerSizeT(menu_size_type::RELATIVE_, 1);
        SizeAttr->LeftOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0.00);
        SizeAttr->TopOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0.00);
        //SizeAttr->XAlignment = menu_region_alignment::CENTER;
        //SizeAttr->YAlignment = menu_region_alignment::CENTER;

        //ConnectNodeToBack(EntityContainer, CreateTextInputNode(Interface));
        //ConnectNodeToBack(EntityContainer, CreateTextInputNode(Interface));
        //ConnectNodeToBack(EntityContainer, CreateTextInputNode(Interface));

        GlobalState->EntitiesPlugin = CreatePlugin(Interface, "Entities");
        AddPlugintoMainMenu(Interface, WindowsDropDownMenu, GlobalState->EntitiesPlugin);

        ConnectNodeToBack(GlobalState->EntitiesPlugin, EntityContainer);
        container_node* EntityWindow = ConnectViaSplitWindow(Interface, DefaultWindow, GlobalState->EntitiesPlugin, 0.3, true, false);
      }
    }
    {
      menu_tree* TestDropDownMenu = CreateNewDropDownMenuItem(GlobalState->World.MenuInterface, "Test");
      {
        // Create Option Window
        container_node* EntityContainer =  NewContainer(Interface, container_type::None);
        color_attribute* BackgroundColor = (color_attribute* ) PushAttribute(Interface, EntityContainer, ATTRIBUTE_COLOR);
        BackgroundColor->Color = V4(0,0.3,0,1);

        container_node* TestPlugin = CreatePlugin(Interface, "Test2");
        AddPlugintoMainMenu(Interface, TestDropDownMenu, TestPlugin);
        
        ConnectNodeToBack(TestPlugin, EntityContainer);
      }
    }
    { // Create some entities
      { // Checker Floor
        ecs::entity_id Entity = NewEntity(GlobalState->World.EntityManager, 0, "Checkered Floor", ecs::flag::RENDER);
        ecs::position::component* Position = GetPositionComponent(&Entity);
        ecs::position::Set(Position, V3(0,-1.1,0),  0, V3(0,1,0));
        ecs::render::component* Render = GetRenderComponent(&Entity);
        Render->MeshHandle = GlobalState->Plane;
        Render->TextureHandle = GlobalState->CheckerBoardTexture;
        Render->Material = ecs::render::GetMaterial(ecs::render::data::MATERIAL_PEARL);
        Render->Scale = V3(10,1,10);
      }

      { // Transparent Cube
        ecs::entity_id Entity = NewEntity(GlobalState->World.EntityManager, 0, "Transparent Cube", ecs::flag::RENDER);
        ecs::position::component* Position = GetPositionComponent(&Entity);
        ecs::position::Set(Position, V3(2,0,0), 0, V3(0,1,0));
        ecs::render::component* Render = GetRenderComponent(&Entity);
        Render->MeshHandle = GlobalState->Cube;
        Render->TextureHandle = GlobalState->WhitePixelTexture;
        Render->Material = ecs::render::GetMaterial(ecs::render::data::MATERIAL_RUBY);
        Render->Scale = V3(1,1,1);
        GlobalState->DebugSquare = PushStruct(GlobalPersistentArena, ecs::entity_id);
        *GlobalState->DebugSquare = Entity;
      }
      
      { // Transparent Cone
        ecs::entity_id Entity = NewEntity(GlobalState->World.EntityManager, 0, "Transparent Cone", ecs::flag::RENDER);
        ecs::position::component* Position = GetPositionComponent(&Entity);
        ecs::position::Set(Position, V3(0,0,2), 0, V3(0,1,0));
        ecs::render::component* Render = GetRenderComponent(&Entity);
        Render->MeshHandle = GlobalState->Cone;
        Render->TextureHandle = GlobalState->WhitePixelTexture;
        Render->Material = ecs::render::GetMaterial(ecs::render::data::MATERIAL_EMERALD);
        Render->Scale = V3(1,1,1);
      }
      
      { // Transparent Sphere
        ecs::entity_id Entity = NewEntity(GlobalState->World.EntityManager, 0, "Transparent Sphere", ecs::flag::RENDER);
        ecs::position::component* Position = GetPositionComponent(&Entity);
        ecs::position::Set(Position, V3(2,0,2), 0, V3(0,1,0));
        ecs::render::component* Render = GetRenderComponent(&Entity);
        Render->MeshHandle = GlobalState->Sphere;
        Render->TextureHandle = GlobalState->WhitePixelTexture;
        Render->Material = ecs::render::GetMaterial(ecs::render::data::MATERIAL_JADE);
        Render->Scale = V3(1,1,1);
      }

      { // Solid Cone
        ecs::entity_id Entity = NewEntity(GlobalState->World.EntityManager, 0, "Solid Cone", ecs::flag::RENDER);
        ecs::position::component* Position = GetPositionComponent(&Entity);
        ecs::position::Set(Position, V3(0,0,0), 0, V3(0,1,0));
        ecs::render::component* Render = GetRenderComponent(&Entity);
        Render->MeshHandle = GlobalState->Cone;
        Render->TextureHandle = GlobalState->WhitePixelTexture;
        Render->Material = ecs::render::GetMaterial(ecs::render::data::MATERIAL_SILVER);
        Render->Scale = V3(1,1,1);
      }
    }
  }else{
    BeginRender(GetRenderSystem());
    ResetRenderGroup(RenderCommands->RenderGroup);
  }

  //Platform.DEBUGPrint("%d, %d, %d\n", Square->EntityID, Square->ChunkListIndex, GetBlockCount(&GlobalState->World.EntityManager->EntityList));

  ecs::position::component* Position = GetPositionComponent(GlobalState->DebugSquare);
  //ecs::position::Set(Position, Position->RelativePosition, RotateQuaternion(0, V3(1,0,0)));
  ecs::position::Set(Position, Position->RelativePosition, QuaternionMultiplication(Position->RelativeRotation, RotateQuaternion(0.01, V3(0,1,0))));

  ecs::render::window_size_pixel* Window = &GlobalState->World.RenderSystem->WindowSize;
  ecs::render::SetWindowSize(GlobalState->World.RenderSystem, RenderCommands);
  CreateFrameBuffer(RenderCommands->RenderGroup, GlobalState->DefaultFrameBuffer,  Window->WindowWidth, Window->WindowHeight, 0, 0, 0, 0);

  GlobalDebugRenderCommands = GlobalState->DebugRenderCommands;
  
  if(!GlobalState->World.MenuInterface->MenuVisible)
  {
    if((ImguiNoneSelected() && ImguiIsInactive())|| ImguiIsDragging())
    {
      SceneInput(&GlobalState->Camera, Input);
    }
  }

  render_group* RenderGroup = RenderCommands->RenderGroup;
  if(jwin::Pushed(Input->Keyboard.Key_ENTER) || Input->ExecutableReloaded)
  {
    ReinitiatePool();
    Platform.DEBUGPrint("We should reload debug code\n");
    CompileShader(RenderGroup,GlobalState->PhongProgram,
      1, LoadFileFromDisk("..\\jwin\\shaders\\PhongVertexCameraView.glsl"),
      1, LoadFileFromDisk("..\\jwin\\shaders\\PhongFragmentCameraView.glsl"));
    CompileShader(RenderGroup,GlobalState->PhongProgramTransparent,
      1, LoadFileFromDisk("..\\jwin\\shaders\\PhongVertexCameraViewTransparent.glsl"),
      1, LoadFileFromDisk("..\\jwin\\shaders\\PhongFragmentCameraViewTransparent.glsl"));
    CompileShader(RenderGroup,GlobalState->PlaneStarProgram,
      1, LoadFileFromDisk("..\\jwin\\shaders\\StarPlaneVertex.glsl"),
      1, LoadFileFromDisk("..\\jwin\\shaders\\StarPlaneFragment.glsl"));
    CompileShader(RenderGroup,GlobalState->SolidColorProgram,
      1, LoadFileFromDisk("..\\jwin\\shaders\\SolidColorVertex.glsl"),
      1, LoadFileFromDisk("..\\jwin\\shaders\\SolidColorFragment.glsl"));
    CompileShader(RenderGroup,GlobalState->EruptionBandProgram,
      1, LoadFileFromDisk("..\\jwin\\shaders\\EruptionBandVertex.glsl"),
      1, LoadFileFromDisk("..\\jwin\\shaders\\EruptionBandFragment.glsl"));
    CompileShader(RenderGroup,GlobalState->TransparentCompositionProgram,
      1, LoadFileFromDisk("..\\jwin\\shaders\\transparent_composition_vertex.glsl"),
      1, LoadFileFromDisk("..\\jwin\\shaders\\transparent_composition_fragment.glsl"));
    CompileShader(RenderGroup, GlobalState->ColoredSquareOverlayProgram, 
      1, LoadFileFromDisk("..\\jwin\\shaders\\ColoredOverlayQuadVertex.glsl"),
      1, LoadFileFromDisk("..\\jwin\\shaders\\ColoredOverlayQuadFragment.glsl"));
    CompileShader(RenderGroup, GlobalState->FontRenterProgram, 
      1, LoadFileFromDisk("..\\jwin\\shaders\\FontRenderVertex.glsl"),
      1, LoadFileFromDisk("..\\jwin\\shaders\\FontRenderFragment.glsl"));
    CompileShader(RenderGroup, GlobalState->GaussianProgramY, 
      1, LoadFileFromDisk("..\\jwin\\shaders\\gaussian_vertex_y.glsl"),
      1, LoadFileFromDisk("..\\jwin\\shaders\\gaussian_fragment_y.glsl"));
    CompileShader(RenderGroup, GlobalState->GaussianProgramX,
      1, LoadFileFromDisk("..\\jwin\\shaders\\gaussian_vertex_x.glsl"),
      1, LoadFileFromDisk("..\\jwin\\shaders\\gaussian_fragment_x.glsl"));
    CompileShader(RenderGroup, GlobalState->TexturedSquareOverlayProgram,
      1, LoadFileFromDisk("..\\jwin\\shaders\\TexturedOverlayQuadVertex.glsl"),
      1, LoadFileFromDisk("..\\jwin\\shaders\\TexturedOverlayQuadFragment.glsl"));
  }

  ecs::position::UpdatePositions(GetEntityManager());
  
#define TMP_STRING_SIZE 128

  UpdateViewMatrix(&GlobalState->Camera);

  
#if 0
  UpdateAndRenderMenuInterface(Input, GetMenuInterface());

  if(!GlobalState->World.MenuInterface->MenuVisible){
    ecs::render::SetDrawWindow(GetRenderSystem(),
      Rect2f(0,0,1,1));
    ecs::render::DrawScene(GetRenderSystem(), GetEntityManager());
  }
#else
  ecs::render::SetDrawWindow(GetRenderSystem(), Rect2f(0,0,1,1));
  ecs::render::DrawScene(GetRenderSystem(), GetEntityManager());
#endif
  ecs::render::NewRenderLevel(GetRenderSystem());
  DrawColorList(&GlobalState->ApplicationImgui);
  ecs::render::NewRenderLevel(GetRenderSystem());
  DrawEntityList(&GlobalState->ApplicationImgui);
  ImguiEnd();
  ecs::render::Draw(GetEntityManager(), GetRenderSystem(), GlobalState->Camera.P, GlobalState->Camera.V);  
} 
