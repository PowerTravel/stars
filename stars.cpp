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
#include "ecs/components/component_position.cpp"
#include "ecs/systems/system_position.cpp"
#include "ecs/systems/system_render.cpp"
#include "menu/menu_interface.cpp"
#include "imgui/imgui.cpp"

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
  Result.PositionNodes = NewChunkList(GlobalPersistentArena, sizeof(ecs::position::position_node), 128);
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


entity_buffer CreateEntityBuffer(memory_arena* Arena)
{
  entity_buffer Result = {};
  Result.NewEntities =  NewChunkList(Arena, sizeof(ecs::entity_id), 16);
  Result.RemovedEntities = NewChunkList(Arena, sizeof(ecs::entity_id), 16);
  Result.Arena = Arena;
  return Result;
}

void Clear(entity_buffer* EntityBuffer)
{
  Clear(&EntityBuffer->NewEntities);
  Clear(&EntityBuffer->RemovedEntities);
}

ecs::entity_id NewEntity(bitmask32 ComponentFlags)
{
  ecs::entity_id Entity = NewEntity(GlobalState->World.EntityManager, ComponentFlags);
  entity_buffer* EntityBuffer = &GlobalState->NewOrRemovedEntityBuffer;
  Push(EntityBuffer->Arena, &EntityBuffer->NewEntities, (bptr) &Entity);
  return Entity;
}

void RemoveEntity(ecs::entity_id Entity)
{
  entity_buffer* EntityBuffer = &GlobalState->NewOrRemovedEntityBuffer;
  Push(EntityBuffer->Arena, &EntityBuffer->RemovedEntities, (bptr) &Entity);
  DeleteEntity(GlobalState->World.EntityManager, &Entity);
}

void AddOrRemoveMenuEntityItems()
{
  entity_buffer* EntityBuffer = &GlobalState->NewOrRemovedEntityBuffer;
  ecs::entity_manager* EntityManager = GlobalState->World.EntityManager;

  container_node* EntityContainer = GlobalState->EntitiesPlugin;
  container_node* Grid = EntityContainer->FirstChild;
  Assert(Grid->Type == container_type::Grid);

  chunk_list_iterator It = BeginIterator(&EntityBuffer->NewEntities);
  while(Valid(&It))
  {
    ecs::entity_id* Entity = (ecs::entity_id*) Next(&It);

    container_node* EntityContainer = ConnectNodeToBack(Grid, NewContainer(GetMenuInterface(), container_type::Grid));

    grid_node* GridNode = GetGridNode(EntityContainer);
    GridNode->Col = 1;
    GridNode->Row = 0;
    GridNode->TotalMarginX = 0.0;
    GridNode->TotalMarginY = 0.0;
    GridNode->Stack = true;
    GridNode->StackXAlignment = menu_region_alignment::LEFT;
    GridNode->StackYAlignment = menu_region_alignment::TOP;
    
    size_attribute* SizeAttr = (size_attribute*) PushAttribute(GetMenuInterface(), EntityContainer, ATTRIBUTE_SIZE);
    SizeAttr->Width = ContainerSizeT(menu_size_type::ABSOLUTE_, 0.15);
    SizeAttr->Height = ContainerSizeT(menu_size_type::ABSOLUTE_, 0.1);
    SizeAttr->LeftOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0.00);
    SizeAttr->TopOffset = ContainerSizeT(menu_size_type::ABSOLUTE_, 0.00);
    alignment_attribute* AlignAttr = (alignment_attribute*) PushAttribute(GetMenuInterface(), EntityContainer, ATTRIBUTE_ALIGNMENT);
    AlignAttr->XAlignment = menu_region_alignment::LEFT;
    AlignAttr->YAlignment = menu_region_alignment::TOP;

    SetColor(GetMenuInterface(), EntityContainer, menu::GetColor(GetColorTable(), "yale blue"));

    container_node* EntityNameNode = ConnectNodeToBack(EntityContainer, NewContainer(GetMenuInterface(), container_type::None));
    text_attribute* EntityNameTextAttr = (text_attribute*) PushAttribute(GetMenuInterface(), EntityNameNode, ATTRIBUTE_TEXT);
    jstr::CopyStringsUnchecked( "Entity: ", EntityNameTextAttr->Text );
    midx Pos = jstr::StringLength(EntityNameTextAttr->Text);
    Pos = jstr::Itoa(Entity->EntityID, 256, EntityNameTextAttr->Text + Pos);
    EntityNameTextAttr->FontSize = 14;
    EntityNameTextAttr->Color = V4(1,1,1,1);

    u32 ComponentArrayCount = ecs::GetComponentCount(GetEntityManager(), Entity);
    u32* ComponentFlags = PushArray(GlobalTransientArena, ComponentArrayCount, u32);
    u32 ComponentCount = ecs::GetComponentTypes(GetEntityManager(), Entity, ComponentFlags);
    Assert(ComponentArrayCount == ComponentCount);
    for (int i = 0; i < ComponentCount; ++i)
    {
      container_node* TextNode = ConnectNodeToBack(EntityContainer, NewContainer(GetMenuInterface(), container_type::None));
      text_attribute* TextAttr = (text_attribute*) PushAttribute(GetMenuInterface(), TextNode, ATTRIBUTE_TEXT);
      jstr::CopyStringsUnchecked(ecs::ComponentTypeToString( (ecs::flag::component_type) ComponentFlags[i]), TextAttr->Text);
      TextAttr->FontSize = 12;
      TextAttr->Color = V4(1,1,1,1);
      switch((ecs::flag::component_type) ComponentFlags[i])
      {
        case ecs::flag::component_type::RENDER:
          {

          }break;
        case ecs::flag::component_type::POSITION:
          {
            container_node* PositionGrid = ConnectNodeToBack(EntityContainer, NewContainer(GetMenuInterface(), container_type::Grid));
            grid_node* PositionGridNode = GetGridNode(PositionGrid);
            PositionGridNode->Col = 3;
            PositionGridNode->Row = 1;
            PositionGridNode->TotalMarginX = 0.0;
            PositionGridNode->TotalMarginY = 0.0;
            PositionGridNode->Stack = true;
            PositionGridNode->StackXAlignment = menu_region_alignment::LEFT;
            PositionGridNode->StackYAlignment = menu_region_alignment::TOP;

            // Interface
            // ecs::position::component* Position = GetPositionComponent(Entity);
            // container_node* VectorInputNode = ConnectNodeToBack(EntityContainer, CreateV3InputContainer(GetMenuInterface()));
            // ConnectToVector(VectorInputNode, &Position->FirstChild->RelativePosition);
            // r' [x.xx, y.yy, z.zz]


            { // X
              container_node* PositionContainer = ConnectNodeToBack(PositionGrid, CreateTextInputNode(GetMenuInterface()));
              text_input_node* PositionTextInputNode = GetTextInputNode(PositionContainer);
              PositionTextInputNode->TextPixelSize = 12;
              ecs::position::component* Position = GetPositionComponent(Entity);
              world_coordinate Pos = Position->FirstChild->RelativePosition;
              char* NumBuf[32] = {};
              jstr::Ftoa( Pos.X, 2, 255, (char*) NumBuf);
              AppendStringToBuffer((utf8_byte*) NumBuf, &PositionTextInputNode->Buffer);
            }

            { // Y
              container_node* PositionContainer = ConnectNodeToBack(PositionGrid, CreateTextInputNode(GetMenuInterface()));
              text_input_node* PositionTextInputNode = GetTextInputNode(PositionContainer);
              PositionTextInputNode->TextPixelSize = 12;
              ecs::position::component* Position = GetPositionComponent(Entity);
              world_coordinate Pos = Position->FirstChild->RelativePosition;
              char* NumBuf[32] = {};
              jstr::Ftoa( Pos.Y, 2, 255, (char*) NumBuf);
              AppendStringToBuffer((utf8_byte*) NumBuf, &PositionTextInputNode->Buffer);
            }

            { // Z
              container_node* PositionContainer = ConnectNodeToBack(PositionGrid, CreateTextInputNode(GetMenuInterface()));
              text_input_node* PositionTextInputNode = GetTextInputNode(PositionContainer);
              PositionTextInputNode->TextPixelSize = 12;
              ecs::position::component* Position = GetPositionComponent(Entity);
              world_coordinate Pos = Position->FirstChild->RelativePosition;
              char* NumBuf[32] = {};
              jstr::Ftoa( Pos.Z, 2, 255, (char*) NumBuf);
              AppendStringToBuffer((utf8_byte*) NumBuf, &PositionTextInputNode->Buffer);
            }

            
          }break;
      }
    }
  }

  Clear(EntityBuffer);
}


r32 GetScrollWheelSize(r32 ListHeight, r32 RowHeight, u32 RowCount, r32 Min, r32 Max)
{
  r32 LinesToFit = ListHeight / RowHeight;
  r32 SizePercentage = LinesToFit / RowCount;
  r32 Result = Clamp(ListHeight * SizePercentage, Min, Max);
  return Result;
}

rect2f GetRowRect(rect2f ListRect, s32 Index, r32 FirstRow, r32 RowHeight)
{
  s32 FirstIndex = (s32) Floor(FirstRow);
  r32 RowOffset = (FirstRow - FirstIndex) * RowHeight;

  v2 RowSize = V2(ListRect.W, RowHeight);

  r32 ListBot =  ListRect.Y;
  r32 ListTop =  ListRect.Y + ListRect.H;

  r32 RowYPos = -(Index+1) * RowHeight + ListTop + RowOffset;
  v2 RowPos = V2(ListRect.X, RowYPos);
  return Rect2f(RowPos,RowSize);
}

v4 GetButtonColor(imgui_id ButtonId, imgui_button_color ButtonColors){
  v4 Color = ButtonColors.InactiveColor;
  if(ImguiIsHot(ButtonId) && ImguiIsActive(ButtonId)) {
    // Button is Highlighted and pressed
    Color = ButtonColors.ActiveAndHotColor;
  }else if(ImguiIsActive(ButtonId)){
    // Button is Pressed
    Color = ButtonColors.ActiveColor;
  }else if(ImguiIsHot(ButtonId)){
    // Button is only highlighted
    Color = ButtonColors.HotColor;
  }
  return Color;
}

struct color_list_data {
  imgui_id* ImguiIDs; // ColorListIndeces
  s32* ColorIDs;      // Mapping IDS from Colors in the ColorTable to list indeces.

  imgui_text_input_buffer TextInputBuffer;
  imgui_scrollable_list ColorList;
  imgui_bordered_window BorderWindow;
};

void DrawColorRow(imgui_context* ImguiContext, imgui_id ButtonID, rect2f RowRect, rect2f ClippedRowRect, u32 ListIndex, void* Data)
{
  color_list_data* ColorListData = (color_list_data*) Data;
  umm ColorIndex = (umm) ColorListData->ColorIDs[ListIndex];
  menu::named_color_hex* NamedColor = menu::GetNamedColor(&GlobalState->ColorTable, (umm) ColorIndex);
  char* ColorName = NamedColor->Name;
  v4 ColorValue   = HexCodeToColorV4(NamedColor->Color);

  v2 Padding = V2(ecs::render::PixelToCanonicalWidth(GetRenderSystem(), 1),ecs::render::PixelToCanonicalHeight(GetRenderSystem(),1));

  r32 RowWidth = RowRect.W;
  r32 ColorSquareWidth = RowRect.H;
  r32 TextWidth = RowRect.W - ColorSquareWidth;

  if(ImguiIsHot(ButtonID) && ImguiIsInactive() && RowRect.H == ClippedRowRect.H){
    r32 TextWidthTmp = ecs::render::GetTextSizeCanonicalSpace(GetRenderSystem(), ImguiContext->FontSize, (utf8_byte*) ColorName).X;
    if(TextWidthTmp > TextWidth)
    {
      TextWidth = TextWidthTmp + 2*Padding.X;
      RowWidth = TextWidthTmp + ColorSquareWidth + 2*Padding.X;
    }
  }

  // Button Background
  v4 ButtonColor = GetButtonColor(ButtonID, ImguiDefaultButtonColor());
  rect2f ButtonBackgroundRect = Rect2f(ClippedRowRect.X, ClippedRowRect.Y, RowWidth, ClippedRowRect.H);
  ecs::render::DrawOverlayQuadCanonicalSpace(GetRenderSystem(), CenteredRect(ButtonBackgroundRect), ButtonColor);

  // Colored Square
  rect2f ColorSquare = Rect2f(ClippedRowRect.X, ClippedRowRect.Y, ColorSquareWidth, ClippedRowRect.H);
  ecs::render::DrawOverlayQuadCanonicalSpace(GetRenderSystem(), CenteredRect(Shrink(ColorSquare,Padding)), ColorValue);

  // Color Name
  rect2f TextRect = Rect2f(RowRect.X + ColorSquareWidth, ClippedRowRect.Y, TextWidth, ClippedRowRect.H);
  r32 DescentOffset = ecs::render::GetCanonicalFontDescenOffset(GetRenderSystem(), ImguiContext->FontSize);
  v2 TextPos = V2(RowRect.X + ColorSquareWidth, RowRect.Y + DescentOffset);
  utf8_string_buffer StringBuffer = SetStringToFit(ImguiContext->FontSize, TextWidth, ColorName);
  ecs::render::DrawTextCanonicalSpace(GetRenderSystem(), TextPos, TextRect, ImguiContext->FontSize, StringBuffer.Buffer, V4(1.0,1.0,1.0,1.0));
}

r32 MouseScroll(u32 RowCount, r32 RowHeight)
{
  r32 Result = 0;
  if(GlobalState->ImguiContext.MouseDZ)
  {
    r32 ScrollTick = 1/20.f;
    r32 TotalListSize = RowHeight * RowCount;
    r32 ScrollTickPercentage = ScrollTick / TotalListSize;
    Result = (GlobalState->ImguiContext.MouseDZ > 0) ? -ScrollTickPercentage : ScrollTickPercentage; 
  }
  return Result;
}

b32 ImguiScrollBarVertical(imgui_scrollable_list* List, rect2f ListRegion, r32 ScrollbarWidth, r32 RowHeight, r32 RowCount)
{
  rect2f ScrollbarRect = Rect2f(ListRegion.X + ListRegion.W - ScrollbarWidth, ListRegion.Y, ScrollbarWidth, ListRegion.H);
  v2 ScrollButtonSize = V2(ScrollbarWidth, GetScrollWheelSize(ListRegion.H, RowHeight, RowCount, 0.03f, ListRegion.H));

  ecs::render::DrawOverlayQuadCanonicalSpace(GetRenderSystem(), CenteredRect(ScrollbarRect), V4(0.5,0.5,0.5,1.0));
  ImguiButton(&GlobalState->ImguiContext, List->VerticalScrollbarId, ScrollbarRect);

  v2 Padding =  V2(ecs::render::PixelToCanonicalWidth(GetRenderSystem(), 1),
                   ecs::render::PixelToCanonicalWidth(GetRenderSystem(), 1));
  
  r32 ScrollWheelPosY = Lerp(List->ScrollAmmount.Y, ScrollbarRect.Y + ScrollbarRect.H - ScrollButtonSize.Y, ScrollbarRect.Y);
  rect2f ScrollWheelRect = Rect2f(ScrollbarRect.X, ScrollWheelPosY, ScrollButtonSize.X, ScrollButtonSize.Y);
  ScrollWheelRect = Shrink(ScrollWheelRect, Padding);
  
  imgui_button_color ButtonColor = ImguiDefaultButtonColor();
  
  if(!ImguiIsHot(List->VerticalScrollbarId))
  {
    ecs::render::DrawOverlayQuadCanonicalSpace(GetRenderSystem(), CenteredRect(ScrollWheelRect), ButtonColor.InactiveColor);
  }else{
    ecs::render::DrawOverlayQuadCanonicalSpace(GetRenderSystem(), CenteredRect(ScrollWheelRect), ButtonColor.HotColor);
  }
  
  v2 MousePos = V2(GlobalState->ImguiContext.MouseX,GlobalState->ImguiContext.MouseY);
  if(ImguiIsActive(List->VerticalScrollbarId)) {
    // Mouse is clickedUp on the scrollbarButton, Cache the mouseDiff.
    if(GlobalState->ImguiContext.ActiveID.idEdge)
    {
      if(Intersects(ScrollWheelRect, MousePos))
      {
        List->ScrollButtonDiff.Y = MousePos.Y - (ScrollbarRect.Y + (1-List->ScrollAmmount.Y) * (ScrollbarRect.H - ScrollButtonSize.Y));  
      }else{
        List->ScrollButtonDiff.Y = ScrollButtonSize.Y*0.5f;
      }
    }else{
      r32 A = ScrollbarRect.Y + List->ScrollButtonDiff.Y;
      r32 B = ScrollbarRect.Y + ScrollbarRect.H - (ScrollButtonSize.Y-List->ScrollButtonDiff.Y);
      List->ScrollAmmount.Y = Unlerp(MousePos.Y, B, A);
    }
  }else{
    if(Intersects(ListRegion,MousePos))
    {
      List->ScrollAmmount.Y += MouseScroll(RowCount, RowHeight);
    }  
  }
  List->ScrollAmmount.Y = Clamp(List->ScrollAmmount.Y, 0,1);

  return ImguiIsActive(List->VerticalScrollbarId);
}


b32 ImguiScrollableButtonList(imgui_scrollable_list* ScrollableList, v2 Pos, v2 Size, u32 RowCount, r32 RowHeight, imgui_id* RowIDs, void* Data, void (RowRenderFunction)(imgui_context* ImguiContext, imgui_id ButtonID, rect2f RowRect, rect2f ClippedRowRect, u32 ListIndex, void* Data)) {

  // List Background
  rect2f BackgroundRect = Rect2f(Pos, Size);
  ecs::render::DrawOverlayQuadCanonicalSpace(GetRenderSystem(), CenteredRect(BackgroundRect), ImguiDefaultButtonColor().InactiveColor);

  r32 LinesToFit = Size.Y / RowHeight;
  v2 ListContentSize = Size;
  r32 StartRow = 0;
  if(LinesToFit < RowCount){
    r32 ScrollbarWidth = 0.01;
    ImguiScrollBarVertical(ScrollableList, BackgroundRect, ScrollbarWidth, RowHeight, RowCount);
    ListContentSize.X -= ScrollbarWidth;
    StartRow = ScrollableList->ScrollAmmount.Y * (RowCount - LinesToFit);
  }

  s32 StartIndex = (s32) Floor(StartRow);

  rect2f ListRect = Rect2f(Pos, ListContentSize);
  b32 Result = 0;
  for (s32 i = 0; i<=LinesToFit; ++i)
  {
    s32 Index = StartIndex + i;
    if(Index < RowCount)
    {
      rect2f RowRect = GetRowRect(ListRect, i, StartRow, RowHeight);
      rect2f ClippedRow = RowRect;
      if(Top(RowRect) > Top(ListRect) || Bot(RowRect) < Bot(ListRect)){
        ClippedRow = Clip(RowRect, Rect2f(Pos, ListContentSize));  
      }

      if(ImguiButton(&GlobalState->ImguiContext, RowIDs[Index], ClippedRow))
      {
        Result = true;    
      }
      RowRenderFunction(&GlobalState->ImguiContext, RowIDs[Index],  RowRect, ClippedRow, Index, Data);
      if(ImguiIsActive(RowIDs[Index]))
      {
        ScrollableList->SelectedRow = Index;
      }
    }
  }
  return Result;
}

struct jimgui_entity_data {
  imgui_id ImguiID;
  ecs::entity_id EntityID;
  b32 Open;
  r32 MenuBoxHeight;
};

struct menu_entity_list {

  imgui_text_input_buffer TextInputBuffer;
  imgui_scrollable_list EntityList;
  imgui_bordered_window BorderWindow;

  
  // Cached menu data
  chunk_list EntityData; // jimgui_entity_data
  rb_tree EntityToDataMap;
};

r32 DrawEntityRow(imgui_context* ImguiContext, v2 TopLeft, rect2f ClipArea, jimgui_entity_data* Data)
{
  // Button Background
  r32 Height = ecs::render::GetLineSpacingCanonicalSpace(GetRenderSystem(), ImguiContext->FontSize);
  r32 ResultHeight = Height;
  //v4 ButtonColor = menu::GetColor(&GlobalState->ColorTable, "taupe");
  rect2f ButtonBackgroundRect = Rect2f(TopLeft.X, TopLeft.Y - Height, ClipArea.W, Height);
//  ecs::render::DrawOverlayQuadCanonicalSpace(GetRenderSystem(), CenteredRect(ButtonBackgroundRect), ButtonColor);


  imgui_button_color ButtonColor = {};
  ButtonColor.InactiveColor = menu::GetColor(&GlobalState->ColorTable, "taupe");
  ButtonColor.ActiveAndHotColor = menu::GetColor(&GlobalState->ColorTable, "persian indigo");
  ButtonColor.ActiveColor = menu::GetColor(&GlobalState->ColorTable, "egyptian blue");
  ButtonColor.HotColor =  menu::GetColor(&GlobalState->ColorTable, "rich black");
  ImguiPlainButton(ImguiContext, Data->ImguiID, ButtonBackgroundRect, ButtonColor);
  if(ImguiIsActive(Data->ImguiID) && ImguiIsHot(Data->ImguiID) && jwin::Released(ImguiContext->LeftMouse))
  {
    Data->Open = !Data->Open;
  }


  rect2f TextRect = Rect2f(ButtonBackgroundRect.X, ButtonBackgroundRect.Y, ButtonBackgroundRect.W, ButtonBackgroundRect.H);
  r32 DescentOffset = ecs::render::GetCanonicalFontDescenOffset(GetRenderSystem(), ImguiContext->FontSize);

  v4 TexCoord = Data->Open ? GlobalImguiContext->Icons.Coordinates[ICON_ANGLE_DOWN] : GlobalImguiContext->Icons.Coordinates[ICON_ANGLE_RIGHT];
  ecs::render::DrawIconCanonicalSpace(GetRenderSystem(), CenteredRect(Rect2f(ButtonBackgroundRect.X, ButtonBackgroundRect.Y, Height,Height)), TexCoord, V4(1,1,1,1));
  v2 TextPos = V2(ButtonBackgroundRect.X + Height, ButtonBackgroundRect.Y + DescentOffset);
  c8 NumBuf[32] = {};
  ecs::entity_id EntityID = Data->EntityID;
  jstr::Itoa(EntityID.EntityID, 31, NumBuf);
  r32 TextWidth = ecs::render::GetTextSizeCanonicalSpace(GetRenderSystem(), ImguiContext->FontSize, (utf8_byte const *) NumBuf).X;
  ecs::render::DrawTextCanonicalSpace(GetRenderSystem(), TextPos, TextRect, ImguiContext->FontSize, (utf8_byte const *) NumBuf, V4(1.0,1.0,1.0,1.0));

  if(Data->Open)
  {
    ecs::position::component* Position = GetPositionComponent(&EntityID);
    if(Position)
    {
      TextPos.Y -= Height;
      ResultHeight += Height;
      world_coordinate Pos = Position->FirstChild->RelativePosition;
      TextPos.X += ecs::render::GetTextSizeCanonicalSpace(GetRenderSystem(), ImguiContext->FontSize, (utf8_byte const *) NumBuf).X + 0.01;
      TextRect = Rect2f(TextPos.X, TextPos.Y, (ClipArea.X + ClipArea.W) - TextPos.X, Height);
      u32 idx = 0;
      c8 NumBuf1[32] = {};
      c8* Scan = NumBuf1;
      if(Pos.X >= 0)
      {
        *Scan++ = ' ';  
      }
      Scan += jstr::Ftoa( Pos.X, 2, 255, Scan);
      *Scan++ = ' ';
      if(Pos.Y >= 0)
      {
        *Scan++ = ' ';  
      }
      Scan += jstr::Ftoa( Pos.Y, 2, 255, Scan);
      *Scan++ = ' ';
      if(Pos.Z >= 0)
      {
        *Scan++ = ' ';
      }
      Scan += jstr::Ftoa( Pos.Z, 2, 255, Scan);

      ecs::render::DrawTextCanonicalSpace(GetRenderSystem(), TextPos, TextRect, ImguiContext->FontSize, (utf8_byte const *) NumBuf1, V4(1.0,1.0,1.0,1.0));
    }
  }else{

  }
  
  return ResultHeight;
}

struct list_map_pair {
  rb_tree* MapToCheckAgainst;
  chunk_list* ResultList;
  memory_arena* Arena;
};

void PopulateWithDataNotInMap(red_black_tree_node const * MenuEntryNode, void* ListMapPairPtr) 
{
  list_map_pair* ListMapPair = (list_map_pair*) ListMapPairPtr;
  rb_tree* MapToCheckAgainst = (rb_tree*) ListMapPair->MapToCheckAgainst;
  chunk_list* ResultList = ListMapPair->ResultList;
  memory_arena* Arena = ListMapPair->Arena;
  midx Key = MenuEntryNode->Key;
  if(Find(MapToCheckAgainst, Key) == 0)
  {
    Push(Arena, ResultList, (bptr) MenuEntryNode->Data->Data);
  }
}

void UpdateListWithEntities(chunk_list* MenuEntityList)
{
  // Insert all current real entities into a search tree
  u32 EntityCount = GetEntityManager()->EntityList.BlockCount;
  u32 MenuEntityCount = MenuEntityList->BlockCount;
  chunk_list ItemsToRemove = NewChunkList(GlobalTransientArena, sizeof(jimgui_entity_data*), EntityCount);
  chunk_list ItemsToAdd    = NewChunkList(GlobalTransientArena, sizeof(ecs::entity*), EntityCount);

  // Create Existing Menu entities search tree
  rb_tree MenuEntitiesMap = NewRBTree(GlobalTransientArena, MenuEntityCount, MenuEntityCount);
  {
    u32 Index = 0;
    chunk_list_iterator IT = BeginIterator(MenuEntityList);
    while(jimgui_entity_data* EntityData = (jimgui_entity_data*) Next(&IT) )
    {
      u32 EntityID = EntityData->EntityID.EntityID;
      Insert(&MenuEntitiesMap, EntityID, (void*) EntityData);
    }
  }

  // Create Existing Entities search tree
  rb_tree ExistingEntitiesMap = NewRBTree(GlobalTransientArena, EntityCount, EntityCount);
  {
    u32 Count = 0;
    chunk_list_iterator IT = BeginIterator(&GetEntityManager()->EntityList);
    while(ecs::entity* Entity = (ecs::entity*) Next(&IT))
    {
      u32 EntityID = Entity->ID.EntityID;
      Insert(&ExistingEntitiesMap, (midx) EntityID, Entity);
    }
  }


  // Fill ItemsToRemove with items in MenuEntitiesMap which are not in ExistingEntitiesMap. 
  list_map_pair LMP1 = {};
  LMP1.MapToCheckAgainst = &ExistingEntitiesMap;
  LMP1.ResultList = &ItemsToRemove;
  LMP1.Arena = GlobalTransientArena;
  PostOrderTraverse(&MenuEntitiesMap.Tree,     (void*) &LMP1, PopulateWithDataNotInMap);

  // Fill ItemsToAdd with items in ExistingEntitiesMap which are not in MenuEntitiesMap. 
  list_map_pair LMP2 = {};
  LMP2.MapToCheckAgainst = &MenuEntitiesMap;
  LMP2.ResultList = &ItemsToAdd;
  LMP2.Arena = GlobalTransientArena;
  PostOrderTraverse(&ExistingEntitiesMap.Tree, (void*) &LMP2, PopulateWithDataNotInMap);

  {
    chunk_list_iterator IT = BeginIterator(&ItemsToRemove);
    while(jimgui_entity_data* EntityData = (jimgui_entity_data*) Next(&IT))
    {
      FreeBlock(MenuEntityList, (bptr) EntityData);
    }
  }

  {
    chunk_list_iterator IT = BeginIterator(&ItemsToAdd);
    while(ecs::entity* EntityData = (ecs::entity*) Next(&IT))
    {
      jimgui_entity_data Data = {};
      Data.ImguiID = NewButtonID();
      Data.EntityID = EntityData->ID;
      Data.Open = 0;
      Data.MenuBoxHeight = 0;
      Push(GlobalPersistentArena, MenuEntityList, (bptr) &Data);
    }
  }
}


b32 ImguiEntityComponentList(imgui_scrollable_list* ScrollableList, v2 Pos, v2 Size, chunk_list* MenuEntityList) {

  // List Background
  rect2f BackgroundRect = Rect2f(Pos, Size);
  ecs::render::DrawOverlayQuadCanonicalSpace(GetRenderSystem(), CenteredRect(BackgroundRect), ImguiDefaultButtonColor().InactiveColor);

  v2 ListContentSize = Size;
  rect2f ListRect = Rect2f(Pos, ListContentSize);
  b32 Result = 0;
  v2 TopLeft = UpperLeftPoint(BackgroundRect);
  s32 Index = 0;

  UpdateListWithEntities(MenuEntityList);
  chunk_list_iterator IT = BeginIterator(MenuEntityList);
  while (jimgui_entity_data* Entity = (jimgui_entity_data*) Next(&IT))
  {
    r32 HeightOfRow = DrawEntityRow(&GlobalState->ImguiContext, TopLeft, BackgroundRect, Entity);
    TopLeft.Y -= HeightOfRow;
  }
  return Result;
}

void DrawEntityList() {
  local_persist menu_entity_list* MenuEntityList = 0;

  if(!MenuEntityList)
  {
    u32 IDCount  = 512;
    u32 EntityChunkCount = 32;
    r32 RowHeight = ecs::render::GetLineSpacingCanonicalSpace(GetRenderSystem(), GlobalState->ImguiContext.FontSize);
    MenuEntityList = PushStruct(GlobalPersistentArena, menu_entity_list);
    MenuEntityList->BorderWindow = ImguiBorderedWindow(Rect2f(V2(0.5,0.5), V2(0.1,0.5)), ecs::render::PixelToCanonicalSpace(GetRenderSystem(), V2(3,3)), RowHeight);
    MenuEntityList->EntityList   = CreateScrollableTextList();
    
    MenuEntityList->EntityData = NewChunkList(GlobalPersistentArena, sizeof(jimgui_entity_data), EntityChunkCount);
    MenuEntityList->EntityToDataMap = NewRBTree(GlobalTransientArena, EntityChunkCount, EntityChunkCount);
  }


  chunk_list_iterator It = BeginIterator(&GetEntityManager()->EntityList);
  while(ecs::entity* Entity = (ecs::entity*) Next(&It))
  {
    ecs::entity_id* EntityID = &Entity->ID;

    u32 ComponentArrayCount = ecs::GetComponentCount(GetEntityManager(), EntityID);
    u32* ComponentFlags = PushArray(GlobalTransientArena, ComponentArrayCount, u32);
    u32 ComponentCount = ecs::GetComponentTypes(GetEntityManager(), EntityID, ComponentFlags);
    Assert(ComponentArrayCount == ComponentCount);

    for (int i = 0; i < ComponentCount; ++i)
    {
      switch((ecs::flag::component_type) ComponentFlags[i])
      {
        case ecs::flag::component_type::RENDER:
        {

        }break;
        case ecs::flag::component_type::POSITION:
          {
            { // X
              ecs::position::component* Position = GetPositionComponent(EntityID);
              world_coordinate Pos = Position->FirstChild->RelativePosition;
              char* NumBuf[32] = {};
              jstr::Ftoa( Pos.X, 2, 255, (char*) NumBuf);
              //AppendStringToBuffer((utf8_byte*) NumBuf, &MenuEntityList->TextInputBufferX.Buffer);
            }

            { // Y
              ecs::position::component* Position = GetPositionComponent(EntityID);
              world_coordinate Pos = Position->FirstChild->RelativePosition;
              char* NumBuf[32] = {};
              jstr::Ftoa( Pos.Y, 2, 255, (char*) NumBuf);
              //AppendStringToBuffer((utf8_byte*) NumBuf, &MenuEntityList->TextInputBufferY.Buffer);
            }

            { // Z
              ecs::position::component* Position = GetPositionComponent(EntityID);
              world_coordinate Pos = Position->FirstChild->RelativePosition;
              char* NumBuf[32] = {};
              jstr::Ftoa( Pos.Z, 2, 255, (char*) NumBuf);
              //AppendStringToBuffer((utf8_byte*) NumBuf, &MenuEntityList->TextInputBufferZ.Buffer);
            }

          }break;
      }
    }
  }

  r32 RowHeight = ecs::render::GetLineSpacingCanonicalSpace(GetRenderSystem(),GlobalState->ImguiContext.FontSize);
  
  v2 ScrollListPos  = V2(MenuEntityList->BorderWindow.Region.X, MenuEntityList->BorderWindow.Region.Y);
  v2 ScrollListSize = V2(MenuEntityList->BorderWindow.Region.W, MenuEntityList->BorderWindow.Region.H - MenuEntityList->BorderWindow.HeaderSize);

  ImguiBorderWindow(&MenuEntityList->BorderWindow, "Entities");

  ImguiEntityComponentList(&MenuEntityList->EntityList, ScrollListPos, ScrollListSize, &MenuEntityList->EntityData);

  if(MenuEntityList->EntityList.SelectedRow >= 0)
  {
    jimgui_entity_data* EntityRow = (jimgui_entity_data*) GetBlockIfItExists(&MenuEntityList->EntityData, MenuEntityList->EntityList.SelectedRow); 
    Assert(EntityRow);
    ecs::entity_id* EntityID = &EntityRow->EntityID;
    ecs::position::component* Position = GetPositionComponent(EntityID);
    world_coordinate Pos = Position->FirstChild->RelativePosition;
    Platform.DEBUGPrint("%d, (%1.2f,%1.2f,%1.2f)\n", EntityID->EntityID, Pos.X, Pos.Y, Pos.Z);

  }
}

void DrawColorList() {

  u32 InputLen = 512;
  u32 ColorCount = GlobalState->ColorTable.ColorCount;
  r32 RowHeight = ecs::render::GetLineSpacingCanonicalSpace(GetRenderSystem(), GlobalState->ImguiContext.FontSize);

  local_persist color_list_data* ColorListData = 0;

  if(!ColorListData)
  {
    ColorListData = PushStruct(GlobalPersistentArena, color_list_data);
    ColorListData->TextInputBuffer = ImguiNewTextInputBuffer(InputLen, PushArray(GlobalPersistentArena, InputLen, utf8_byte));
    ColorListData->ImguiIDs         = PushArray(GlobalPersistentArena, ColorCount, imgui_id);
    ColorListData->ColorIDs         = PushArray(GlobalPersistentArena, ColorCount, s32);

    ColorListData->ColorList = CreateScrollableTextList();
    ColorListData->BorderWindow = ImguiBorderedWindow(Rect2f(V2(0.1,0.25), V2(0.1,0.5)), ecs::render::PixelToCanonicalSpace(GetRenderSystem(), V2(3,3)), RowHeight);
    for (int i = 0; i < ColorCount; ++i)
    {
      ColorListData->ImguiIDs[i] = NewButtonID();
    }
  }

  s32 RowCount = 0;
  ZeroArray(ColorCount, ColorListData->ColorIDs);
  imgui_id* ImguiIDs = PushArray(GlobalPersistentArena, ColorCount, imgui_id);
  for (u32 i = 0; i < ColorCount; ++i) {
    menu::named_color_hex* NamedColor = menu::GetNamedColor(&GlobalState->ColorTable, (umm) i);
    char ColorNameLower[512] = {};
    Utf8ToLower( (utf8_byte*) NamedColor->Name, (utf8_byte*) ColorNameLower);
    char InputStringLower[512] = {};
    Utf8ToLower( ColorListData->TextInputBuffer.Buffer.Buffer, (utf8_byte*) InputStringLower);
    if(ColorListData->TextInputBuffer.CharCount == 0 || jstr::Contains( InputStringLower, ColorNameLower))
    {
      ColorListData->ColorIDs[RowCount] = i;
      ImguiIDs[RowCount] = ColorListData->ImguiIDs[i];
      RowCount++;
    }
  }

  imgui_bordered_window* BorderWindow = &ColorListData->BorderWindow;

  // SearchIcon
  v4 SearchBoxBackgroundColor = menu::GetColor(&GlobalState->ColorTable, "bole");
  v2 SearchIconPos = V2(BorderWindow->Region.X, BorderWindow->Region.Y);
  v2 SearchIconSize = V2(RowHeight, RowHeight);
  v4 TexCoord = GlobalImguiContext->Icons.Coordinates[ICON_SEARCH];
  rect2f SearchIconRectBackground = Rect2f(SearchIconPos, SearchIconSize);
  ecs::render::DrawOverlayQuadCanonicalSpace(GetRenderSystem(), CenteredRect(SearchIconRectBackground), SearchBoxBackgroundColor);
  rect2f SearchIconRect = Shrink(SearchIconRectBackground, 0.1*SearchIconRectBackground.W);
  ecs::render::DrawIconCanonicalSpace(GetRenderSystem(), CenteredRect(SearchIconRect),  TexCoord, V4(1,1,1,1));

  
  v2 FilterBarDialogPos  = V2(BorderWindow->Region.X + RowHeight, BorderWindow->Region.Y);
  v2 FilterBarDialogSize = V2(BorderWindow->Region.W - RowHeight, RowHeight);
  if(ImguiTextDialog(&ColorListData->TextInputBuffer, ColorListData->TextInputBuffer.ID, FilterBarDialogPos, FilterBarDialogSize, SearchBoxBackgroundColor))
  {
    if(ImguiIsSelected(ColorListData->TextInputBuffer.ID))
    {
      ImguiReadInput(&ColorListData->TextInputBuffer, GlobalInput);
    }
  }

  v2 ScrollListPos  = V2(BorderWindow->Region.X, BorderWindow->Region.Y + RowHeight);
  v2 ScrollListSize = V2(BorderWindow->Region.W, BorderWindow->Region.H - 2* RowHeight);
  
  ImguiBorderWindow(BorderWindow, "Colors");

  if(ImguiScrollableButtonList(&ColorListData->ColorList, ScrollListPos, ScrollListSize, RowCount, RowHeight, ImguiIDs, (void*) ColorListData, DrawColorRow))
  {
    if(GlobalState->ImguiContext.ActiveID.idEdge)
    {
      menu::named_color_hex* NamedColor = menu::GetNamedColor(&GlobalState->ColorTable, ColorListData->ColorIDs[ColorListData->ColorList.SelectedRow] );
      v4 Color =  HexCodeToColorV4(NamedColor->Color);
      v4 HexColor = 255 * Color;
      Platform.DEBUGPrint("V4(%f, %f, %f, %f) - %s\n", Color.X, Color.Y, Color.Z, Color.W, 
        NamedColor->Name);  
    }
  }
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
    GlobalState->NewOrRemovedEntityBuffer = CreateEntityBuffer(GlobalPersistentArena);
    { // Create some entities
      { // Checker Floor
        ecs::entity_id Entity = NewEntity(ecs::flag::RENDER);
        ecs::position::component* Position = GetPositionComponent(&Entity);
        InitiatePositionComponent(Position, V3(0,-1.1,0), 0);
        ecs::render::component* Render = GetRenderComponent(&Entity);
        Render->MeshHandle = GlobalState->Plane;
        Render->TextureHandle = GlobalState->CheckerBoardTexture;
        Render->Material = ecs::render::GetMaterial(ecs::render::data::MATERIAL_PEARL);
        Render->Scale = V3(10,1,10);
      }

      { // Transparent Cube
        ecs::entity_id Entity = NewEntity(ecs::flag::RENDER);
        ecs::position::component* Position = GetPositionComponent(&Entity);
        InitiatePositionComponent(Position, V3(2,0,0), 0);
        ecs::render::component* Render = GetRenderComponent(&Entity);
        Render->MeshHandle = GlobalState->Cube;
        Render->TextureHandle = GlobalState->WhitePixelTexture;
        Render->Material = ecs::render::GetMaterial(ecs::render::data::MATERIAL_RUBY);
        Render->Scale = V3(1,1,1);
      }
      
      { // Transparent Cone
        ecs::entity_id Entity = NewEntity(ecs::flag::RENDER);
        ecs::position::component* Position = GetPositionComponent(&Entity);
        InitiatePositionComponent(Position, V3(0,0,2), 0);
        ecs::render::component* Render = GetRenderComponent(&Entity);
        Render->MeshHandle = GlobalState->Cone;
        Render->TextureHandle = GlobalState->WhitePixelTexture;
        Render->Material = ecs::render::GetMaterial(ecs::render::data::MATERIAL_EMERALD);
        Render->Scale = V3(1,1,1);
      }
      
      { // Transparent Sphere
        ecs::entity_id Entity = NewEntity(ecs::flag::RENDER);
        ecs::position::component* Position = GetPositionComponent(&Entity);
        InitiatePositionComponent(Position, V3(2,0,2), 0);
        ecs::render::component* Render = GetRenderComponent(&Entity);
        Render->MeshHandle = GlobalState->Sphere;
        Render->TextureHandle = GlobalState->WhitePixelTexture;
        Render->Material = ecs::render::GetMaterial(ecs::render::data::MATERIAL_JADE);
        Render->Scale = V3(1,1,1);
      }

      { // Solid Cone
        ecs::entity_id Entity = NewEntity(ecs::flag::RENDER);
        ecs::position::component* Position = GetPositionComponent(&Entity);
        InitiatePositionComponent(Position, V3(0,0,0), 0);
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

  AddOrRemoveMenuEntityItems();

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
  DrawColorList();
  ecs::render::NewRenderLevel(GetRenderSystem());
  DrawEntityList();
  ImguiEnd();
  ecs::render::Draw(GetEntityManager(), GetRenderSystem(), GlobalState->Camera.P, GlobalState->Camera.V);  
} 
