#include "stars.h"

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
#include "containers/chunk_list.cpp"
#include "containers/linked_memory.cpp"
#include "ecs/entity_components_backend.cpp"
#include "ecs/entity_components.cpp"
#include "ecs/systems/system_position.cpp"
#include "ecs/systems/system_render.cpp"
#include "imgui/imgui.cpp"
#include "imgui/application_imgui.cpp"
#include "broad_phase_collision_tree.cpp"
#include "asset_manager/asset_manager.cpp"
#include "mappers/obj_to_gl_mesh.h"
#include "dynamic_aabb_tree.cpp"
#include "ecs/components/component_collider.h"
//#include "dynamic_aabb_tree.cpp"
//#include "externals\json_fwd.hpp"
#include "gltf/gltf_loader.h"
#include "gltf/gltf_mapper.h"

#include "utils.h"

#define SPOTCOUNT 200

global_variable r32 g_t = 0;



file_local inline void Initiate(asset::mesh* Mesh, asset::phong_material* Material, ecs::render::component* Render)
{
  u32 MeshAssetKey = asset::ToHeader( (bptr) Mesh)->Key;
  u32 MaterialAssetKey = asset::ToHeader( (bptr) Material)->Key;
  ecs::render::Init(MeshAssetKey, MaterialAssetKey, Render);
}

file_local inline void Initiate(asset::render_group_element* Element, ecs::render::component* Render)
{
  if(Element->Material)
  {
    Initiate(Element->Mesh, Element->Material, Render);
  }else{
    Initiate(Element->Mesh, (asset::phong_material*) asset::Find(asset::type::PHONG_MATERIAL, "silver"), Render);
  }
  //Initiate(Element->Mesh, (asset::phong_material*) asset::Find(asset::type::PHONG_MATERIAL, "silver"), Render);
}

file_local inline void Initiate(u32 RenderGroupAssetHandle, ecs::render::component* Render)
{
  asset::render_group* Grp = (asset::render_group*) asset::Find(asset::type::RENDER_GROUP, RenderGroupAssetHandle);
  Assert(Grp->ElementCount == 1); // We don't support rendering mutliple Elements
  Initiate(Grp->Elements->Mesh, Grp->Elements->Material, Render);
}

void LoadMaterial(u32 MapKdHandle, v4 Ambient, v4 Diffuse, v4 Specular, r32 Shininess, const c8* UniqueName)
{
  asset::phong_material Material = {};
  Material.Ka = &Ambient;
  Material.Kd = &Diffuse;
  Material.Ks = &Specular;
  Material.Ns = &Shininess;
  Material.MapKdHandle = MapKdHandle;
  asset::LoadMaterial(UniqueName, &Material);
}

void LoadMaterials()
{
  u8 WhitePixel[4] = {255,255,255,255};
  void* WhitePixelPtr = PushCopy(GlobalTransientArena, sizeof(WhitePixel), (void*) WhitePixel);
  asset::image WhitePixelBitmap = {};
  WhitePixelBitmap.Channels = 4;
  WhitePixelBitmap.Width = 1;
  WhitePixelBitmap.Height = 1;
  WhitePixelBitmap.Pixels = (bptr) WhitePixelPtr;
  u32 TextureKey = 0;
  asset::LoadImage("WhitePixel", "N/A", "N/A", &WhitePixelBitmap, &TextureKey);
  ecs::render::LoadImageToGpu(TextureKey, &WhitePixelBitmap);


  LoadMaterial(TextureKey, {0.0215f,    0.1745f,    0.0215f,   0.55f}, {0.07568f,    0.61424f,    0.07568f,    0.55f}, {0.633f,       0.727811f,    0.633f,      0.55f}, 128 * 0.6f,          "emerald");
  LoadMaterial(TextureKey, {0.135f,     0.2225f,    0.1575f,   0.95f}, {0.54f,       0.89f,       0.63f,       0.95f}, {0.316228f,    0.316228f,    0.316228f,   0.95f}, 128 * 0.1f,          "jade");
  LoadMaterial(TextureKey, {0.05375f,   0.05f,      0.06625f,  0.82f}, {0.18275f,    0.17f,       0.22525f,    0.82f}, {0.332741f,    0.328634f,    0.346435f,   0.82f}, 128 * 0.3f,          "obsidian");
  LoadMaterial(TextureKey, {0.25f,      0.20725f,   0.20725f,  1.00f}, {1.0f,        0.829f,      0.829f,      1.00f}, {0.296648f,    0.296648f,    0.296648f,   1.00f}, 128 * 0.088f,        "pearl");
  LoadMaterial(TextureKey, {0.1745f,    0.01175f,   0.01175f,  0.55f}, {0.61424f,    0.04136f,    0.04136f,    0.55f}, {0.727811f,    0.626959f,    0.626959f,   0.55f}, 128 * 0.6f,          "ruby");
  LoadMaterial(TextureKey, {0.1f,       0.18725f,   0.1745f,   0.80f}, {0.396f,      0.74151f,    0.69102f,    0.80f}, {0.297254f,    0.30829f,     0.306678f,   0.80f}, 128 * 0.1f,          "turquoise");
  LoadMaterial(TextureKey, {0.329412f,  0.223529f,  0.027451f, 1.00f}, {0.780392f,   0.568627f,   0.113725f,   1.00f}, {0.992157f,    0.941176f,    0.807843f,   1.00f}, 128 * 0.21794872f,   "brass");
  LoadMaterial(TextureKey, {0.2125f,    0.1275f,    0.054f,    1.00f}, {0.714f,      0.4284f,     0.18144f,    1.00f}, {0.393548f,    0.271906f,    0.166721f,   1.00f}, 128 * 0.2f,          "bronze");
  LoadMaterial(TextureKey, {0.105882f, 0.058824f, 0.113725f,   1.00f}, {0.427451f,   0.470588f,   0.541176f,   1.00f}, {0.333333f,    0.333333f,    0.521569f,  1.0f },  9.84615f,            "tin");
  LoadMaterial(TextureKey, {0.25f,     0.148f,    0.06475f,    1.00f}, {0.4f,        0.2368f,     0.1036f,     1.00f}, {0.774597f,    0.458561f,    0.200621f,  1.0f },  76.8f,               "polished_bronze");
  LoadMaterial(TextureKey, {0.25f,      0.25f,      0.25f,     1.00f}, {0.4f,        0.4f,        0.4f,        1.00f}, {0.774597f,    0.774597f,    0.774597f,   1.00f}, 128 * 0.6f,          "chrome");
  LoadMaterial(TextureKey, {0.19125f,   0.0735f,    0.0225f,   1.00f}, {0.7038f,     0.27048f,    0.0828f,     1.00f}, {0.256777f,    0.137622f,    0.086014f,   1.00f}, 128 * 0.1f,          "copper");
  LoadMaterial(TextureKey, {0.2295f,   0.08825f,  0.0275f,     1.00f}, {0.5508f,     0.2118f,     0.066f,      1.00f}, {0.580594f,    0.223257f,    0.0695701f, 1.0f },  51.2f,               "polished_copper");
  LoadMaterial(TextureKey, {0.24725f,   0.1995f,    0.0745f,   1.00f}, {0.75164f,    0.60648f,    0.22648f,    1.00f}, {0.628281f,    0.555802f,    0.366065f,   1.00f}, 128 * 0.4f,          "gold");
  LoadMaterial(TextureKey, {0.24725f,  0.2245f,   0.0645f,     1.00f}, {0.34615f,    0.3143f,     0.0903f,     1.00f}, {0.797357f,    0.723991f,    0.208006f,  1.0f},   83.2f,               "polished_gold");
  LoadMaterial(TextureKey, {0.19225f,   0.19225f,   0.19225f,  1.00f}, {0.50754f,    0.50754f,    0.50754f,    1.00f}, {0.508273f,    0.508273f,    0.508273f,   1.00f}, 128 * 0.4f,          "silver");
  LoadMaterial(TextureKey, {0.23125f,  0.23125f,  0.23125f,    1.00f}, {0.2775f,     0.2775f,     0.2775f,     1.00f}, {0.773911f,    0.773911f,    0.773911f,  1.0f },  89.6f,               "polished_silver ");
  LoadMaterial(TextureKey, {0.0f,       0.0f,       0.0f,      1.00f}, {0.01f,       0.01f,       0.01f,       1.00f}, {0.50f,        0.50f,        0.50f,       1.00f}, 128 * 0.25f,         "black_plastic");
  LoadMaterial(TextureKey, {0.0f,       0.1f,       0.06f,     1.00f}, {0.0f,        0.50980392f, 0.50980392f, 1.00f}, {0.50196078f,  0.50196078f,  0.50196078f, 1.00f}, 128 * 0.25f,         "cyan_plastic");
  LoadMaterial(TextureKey, {0.0f,       0.0f,       0.0f,      1.00f}, {0.1f,        0.35f,       0.1f,        1.00f}, {0.45f,        0.55f,        0.45f,       1.00f}, 128 * 0.25f,         "green_plastic");
  LoadMaterial(TextureKey, {0.0f,       0.0f,       0.0f,      1.00f}, {0.5f,        0.0f,        0.0f,        1.00f}, {0.7f,         0.6f,         0.6f,        1.00f}, 128 * 0.25f,         "red_plastic");
  LoadMaterial(TextureKey, {0.0f,       0.0f,       0.0f,      1.00f}, {0.55f,       0.55f,       0.55f,       1.00f}, {0.70f,        0.70f,        0.70f,       1.00f}, 128 * 0.25f,         "white_plastic");
  LoadMaterial(TextureKey, {0.0f,       0.0f,       0.0f,      1.00f}, {0.5f,        0.5f,        0.0f,        1.00f}, {0.60f,        0.60f,        0.50f,       1.00f}, 128 * 0.25f,         "yellow_plastic");
  LoadMaterial(TextureKey, {0.02f,      0.02f,      0.02f,     1.00f}, {0.01f,       0.01f,       0.01f,       1.00f}, {0.4f,         0.4f,         0.4f,        1.00f}, 128 * 0.078125f,     "black_rubber");
  LoadMaterial(TextureKey, {0.0f,       0.05f,      0.05f,     1.00f}, {0.4f,        0.5f,        0.5f,        1.00f}, {0.04f,        0.7f,         0.7f,        1.00f}, 128 * 0.078125f,     "cyan_rubber");
  LoadMaterial(TextureKey, {0.0f,       0.05f,      0.0f,      1.00f}, {0.4f,        0.5f,        0.4f,        1.00f}, {0.04f,        0.7f,         0.04f,       1.00f}, 128 * 0.078125f,     "green_rubber");
  LoadMaterial(TextureKey, {0.05f,      0.0f,       0.0f,      1.00f}, {0.5f,        0.4f,        0.4f,        1.00f}, {0.7f,         0.04f,        0.04f,       1.00f}, 128 * 0.078125f,     "red_rubber");
  LoadMaterial(TextureKey, {0.05f,      0.05f,      0.05f,     1.00f}, {0.5f,        0.5f,        0.5f,        1.00f}, {0.7f,         0.7f,         0.7f,        1.00f}, 128 * 0.078125f,     "white_rubber");
  LoadMaterial(TextureKey, {0.05f,      0.05f,      0.0f,      1.00f}, {0.5f,        0.5f,        0.4f,        1.00f}, {0.7f,         0.7f,         0.04f,       1.00f}, 128 * 0.078125f,     "yellow_rubber");
}

u32 CreateLineRenderProgram(render_group* RenderGroup)
{
  u32 ProgramHandle = NewShaderProgram(RenderGroup, "SolidLineProgram");
  AddUniform(RenderGroup, UniformType::M4,  ProgramHandle, "ProjectionMat");
  AddVarying(RenderGroup, UniformType::V3,  ProgramHandle, "P0");
  AddVarying(RenderGroup, UniformType::V3,  ProgramHandle, "P1");
  AddVarying(RenderGroup, UniformType::V4,  ProgramHandle, "Color_in");
  AddVarying(RenderGroup, UniformType::R32, ProgramHandle, "Thickness");
  CompileShader(RenderGroup, ProgramHandle,
      1, LoadFileFromDisk("..\\jwin\\shaders\\SolidLineProgramVertex.glsl"),
      1, LoadFileFromDisk("..\\jwin\\shaders\\SolidLineProgramFragment.glsl"));
  return ProgramHandle;
}

u32 Load32BitColorTexture(const c8* Name, const c8* Path)
{
  u32 Key = asset::LoadTga(Path, Name);
  asset::image* Image = (asset::image*) asset::Find(asset::type::IMAGE, Key);
  u32 Handle = ecs::render::LoadImageToGpu(Key, Image);
  return Handle;
}


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

u32 CreatePhongNoTexProgram(render_group* RenderGroup)
{
  u32 ProgramHandle = NewShaderProgram(RenderGroup,
      "PhongShadingNoTex");

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
      1, LoadFileFromDisk("..\\jwin\\shaders\\PhongVertexCameraViewNoTex.glsl"),
      1, LoadFileFromDisk("..\\jwin\\shaders\\PhongFragmentCameraViewNoTex.glsl"));

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
  RayObj->MeshHandle = ecs::render::GetMeshHandle("Cone");
  RayObj->FrameBufferHandle = ecs::render::FrameBuffer(ecs::render::data::FRAMEBUFFER_TRANSPARENT);

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
  Ray->MeshHandle = ecs::render::GetMeshHandle("Triangle");
  Ray->FrameBufferHandle = ecs::render::FrameBuffer(ecs::render::data::FRAMEBUFFER_TRANSPARENT);

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
  Eruptions->MeshHandle = ecs::render::GetMeshHandle("Sphere");
  Eruptions->FrameBufferHandle = ecs::render::FrameBuffer(ecs::render::data::FRAMEBUFFER_MSAA);;
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
    Sphere1->MeshHandle = ecs::render::GetMeshHandle("Sphere");
    Sphere1->FrameBufferHandle = ecs::render::FrameBuffer(ecs::render::data::FRAMEBUFFER_MSAA);;
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
    Sphere2->MeshHandle = ecs::render::GetMeshHandle("Sphere");
    Sphere2->FrameBufferHandle = ecs::render::FrameBuffer(ecs::render::data::FRAMEBUFFER_MSAA);;
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
    Sphere3->MeshHandle = ecs::render::GetMeshHandle("Sphere");
    Sphere3->FrameBufferHandle = ecs::render::FrameBuffer(ecs::render::data::FRAMEBUFFER_MSAA);;
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
    Sphere4->MeshHandle = ecs::render::GetMeshHandle("Sphere");
    Sphere4->FrameBufferHandle = ecs::render::FrameBuffer(ecs::render::data::FRAMEBUFFER_MSAA);;
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
    Halo->MeshHandle = ecs::render::GetMeshHandle("Plane");
    Halo->FrameBufferHandle = ecs::render::FrameBuffer(ecs::render::data::FRAMEBUFFER_TRANSPARENT);

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

world InitiateWorld(application_render_commands* RenderCommands)
{
  world Result = {};
  Result.EntityManager = ecs::CreateEntityManager();
  Result.RenderSystem = ecs::render::CreateRenderSystem(RenderCommands->RenderGroup, RenderCommands->WindowInfo.Width, RenderCommands->WindowInfo.Height, RenderCommands);

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
    if(jwin::Active(Input->Keyboard.Key_LSHIFT))
    {
      CamSpeed = 1;
    }
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

void DrawAllRenderObjects()
{
  ecs::filtered_entity_iterator EntityIterator = GetComponentsOfType(GlobalEntityManager, ecs::flag::RENDER);
  while(Next(&EntityIterator))
  {
    ecs::render::component* Component = GetRenderComponent(&EntityIterator);
    DrawRenderObject(Component);
  }
}

struct overlay_doodad {
  v3 Pos;
  quat Rot;
  r32 ScaleFraction;
};

void DrawDoodad(m4 ProjectionMatrix, m4 ViewMatrix, void* Data)
{
  render_object* Object = PushNewRenderObject(GlobalRenderSystem->RenderGroup);
  Object->ProgramHandle = GlobalState->PhongShadingNoTexProgram;
  Object->FrameBufferHandle = ecs::render::FrameBuffer(ecs::render::data::FRAMEBUFFER_MSAA);
  Object->MeshHandle = ecs::render::GetMeshHandle("Cube");

  overlay_doodad* Doodad = (overlay_doodad*) Data;
 
  m4 CamToWorld = RigidInverse(ViewMatrix);
  v3 CamPosition = V3(Column(CamToWorld,3));  
  r32 Scale = 2*Norm(Doodad->Pos-CamPosition) * Doodad->ScaleFraction;

  m4 ScaleMat = GetScaleMatrix(V4(Scale, Scale, Scale,1));
  m4 RotationMat = GetRotationMatrix(Doodad->Rot);
  m4 TranslationMat = GetTranslationMatrix(V4(Doodad->Pos,1));
  m4 ModelMat = TranslationMat*RotationMat*ScaleMat;

  m4 ModelView = ViewMatrix*ModelMat;
  m4 NormalView = Transpose(RigidInverse(ModelView));

  v3 LightPosition = V3(1,1,1);
  v3 LightDirection = V3(Transpose(RigidInverse(ViewMatrix)) * V4(LightPosition,0));

  v4 Ambient = V4(0.05f,      0.0f,       0.0f,  1.00f);
  v4 Diffuse = V4(0.5f,        0.4f,      0.4f,  1.00f);
  v4 Specular = V4(0.7f,         0.04f,   0.04f, 1.00f);
  r32 Shininess = 128 * 0.078125f;
  
  PushUniform(Object, GetUniformHandle(GlobalRenderSystem->RenderGroup, Object->ProgramHandle, "ProjectionMat"),    ProjectionMatrix);
  PushUniform(Object, GetUniformHandle(GlobalRenderSystem->RenderGroup, Object->ProgramHandle, "ModelView"),        ModelView);
  PushUniform(Object, GetUniformHandle(GlobalRenderSystem->RenderGroup, Object->ProgramHandle, "NormalView"),       NormalView);
  PushUniform(Object, GetUniformHandle(GlobalRenderSystem->RenderGroup, Object->ProgramHandle, "LightDirection"),   LightDirection);
  PushUniform(Object, GetUniformHandle(GlobalRenderSystem->RenderGroup, Object->ProgramHandle, "LightColor"),       V3(1,1,1));
  PushUniform(Object, GetUniformHandle(GlobalRenderSystem->RenderGroup, Object->ProgramHandle, "MaterialAmbient"),  Ambient);
  PushUniform(Object, GetUniformHandle(GlobalRenderSystem->RenderGroup, Object->ProgramHandle, "MaterialDiffuse"),  Diffuse);
  PushUniform(Object, GetUniformHandle(GlobalRenderSystem->RenderGroup, Object->ProgramHandle, "MaterialSpecular"), Specular);
  PushUniform(Object, GetUniformHandle(GlobalRenderSystem->RenderGroup, Object->ProgramHandle, "Shininess"),        Shininess);
}


struct overlay_aabb {
  aabb3f a;
  v4 Ambient;
  v4 Diffuse;
  v4 Specular;
  r32 Shininess;
};

void DrawAABBBoxOutline(m4 ProjectionMatrix, m4 ViewMatrix, void* Data)
{
  render_object* Object = PushNewRenderObject(GlobalRenderSystem->RenderGroup);
  Object->ProgramHandle = GlobalState->PhongShadingNoTexProgram;
  Object->FrameBufferHandle = ecs::render::FrameBuffer(ecs::render::data::FRAMEBUFFER_MSAA);
  Object->MeshHandle = ecs::render::GetMeshHandle("Cube");

  overlay_doodad* Doodad = (overlay_doodad*) Data;
 
  m4 CamToWorld = RigidInverse(ViewMatrix);
  v3 CamPosition = V3(Column(CamToWorld,3));  
  r32 Scale = 2*Norm(Doodad->Pos-CamPosition) * Doodad->ScaleFraction;

  m4 ScaleMat = GetScaleMatrix(V4(Scale, Scale, Scale,1));
  m4 RotationMat = GetRotationMatrix(Doodad->Rot);
  m4 TranslationMat = GetTranslationMatrix(V4(Doodad->Pos,1));
  m4 ModelMat = TranslationMat*RotationMat*ScaleMat;

  m4 ModelView = ViewMatrix*ModelMat;
  m4 NormalView = Transpose(RigidInverse(ModelView));

  v3 LightPosition = V3(1,1,1);
  v3 LightDirection = V3(Transpose(RigidInverse(ViewMatrix)) * V4(LightPosition,0));
  
  v4 Ambient = V4(0.05f,      0.0f,       0.0f,  1.00f);
  v4 Diffuse = V4(0.5f,        0.4f,      0.4f,  1.00f);
  v4 Specular = V4(0.7f,         0.04f,   0.04f, 1.00f);
  r32 Shininess = 128 * 0.078125f;

  PushUniform(Object, GetUniformHandle(GlobalRenderSystem->RenderGroup, Object->ProgramHandle, "ProjectionMat"),    ProjectionMatrix);
  PushUniform(Object, GetUniformHandle(GlobalRenderSystem->RenderGroup, Object->ProgramHandle, "ModelView"),        ModelView);
  PushUniform(Object, GetUniformHandle(GlobalRenderSystem->RenderGroup, Object->ProgramHandle, "NormalView"),       NormalView);
  PushUniform(Object, GetUniformHandle(GlobalRenderSystem->RenderGroup, Object->ProgramHandle, "LightDirection"),   LightDirection);
  PushUniform(Object, GetUniformHandle(GlobalRenderSystem->RenderGroup, Object->ProgramHandle, "LightColor"),       V3(1,1,1));
  PushUniform(Object, GetUniformHandle(GlobalRenderSystem->RenderGroup, Object->ProgramHandle, "MaterialAmbient"),  Ambient);
  PushUniform(Object, GetUniformHandle(GlobalRenderSystem->RenderGroup, Object->ProgramHandle, "MaterialDiffuse"),  Diffuse);
  PushUniform(Object, GetUniformHandle(GlobalRenderSystem->RenderGroup, Object->ProgramHandle, "MaterialSpecular"), Specular);
  PushUniform(Object, GetUniformHandle(GlobalRenderSystem->RenderGroup, Object->ProgramHandle, "Shininess"),        Shininess);
}

void DrawOverlayObject(v3 Pos)
{
  overlay_doodad* Doodad = PushStruct(GlobalTransientArena, overlay_doodad);
  Doodad->Pos = Pos;
  Doodad->Rot = Quaternion();
  Doodad->ScaleFraction =  0.007;
  ecs::render::DrawOverlay3DObject((void*) Doodad, DrawDoodad);
}

void DrawOverlayObjects()
{
  ecs::filtered_entity_iterator EntityIterator = GetComponentsOfType(GlobalEntityManager, ecs::flag::POSITION);
  while(Next(&EntityIterator))
  {
    ecs::position::component* Component = GetPositionComponent(&EntityIterator);
    DrawOverlayObject(Component->RelativePosition);
  }
}

void PowerOfTwoMiddles(u32 MaxNum){
  u32 PowTwo_1 = 2;
  u32 PowTwo_2 = 4;
  u32 Mid = (PowTwo_1 + PowTwo_2)/2;
  while(Mid <= MaxNum)
  {
    Platform.DEBUGPrint("%d\n", Mid);
    PowTwo_1 = PowTwo_2;
    PowTwo_2 *= 2;
    Mid = (PowTwo_1 + PowTwo_2)/2;
  }
}

// void ApplicationUpdateAndRender(application_memory* Memory, application_render_commands* RenderCommands, jwin::device_input* Input)
extern "C" JWIN_UPDATE_AND_RENDER(ApplicationUpdateAndRender)
{
  GlobalState          = JwinBeginFrameMemory(application_state);
  GlobalInput          = Input;
  GlobalImguiContext   = &GlobalState->ImguiContext;
  GlobalRenderCommands = RenderCommands;
  GlobalRenderSystem   = GlobalState->World.RenderSystem;
  GlobalAssetManager   = GlobalState->AssetManager;
  GlobalEntityManager  = GlobalState->World.EntityManager;

  ResetRenderGroup(RenderCommands->RenderGroup);
  platform_offscreen_buffer* OffscreenBuffer = &RenderCommands->PlatformOffscreenBuffer;
  ImguiBegin(Input);
  g_t = Input->Time;

  if(!GlobalState->Initialized)
  {
    PowerOfTwoMiddles(1000000000);
    GlobalState->ColorTable = menu::CreateColorTable(GlobalPersistentArena);
    GlobalState->AssetManager = asset::CreateAssetManager();
    GlobalAssetManager = GlobalState->AssetManager;

    RenderCommands->RenderGroup = InitiateRenderGroup();
    GlobalState->World  = InitiateWorld(RenderCommands);
    GlobalRenderSystem  = GlobalState->World.RenderSystem;
    GlobalEntityManager = GlobalState->World.EntityManager;


    ecs::render::window_size_pixel* Window = &GlobalState->World.RenderSystem->WindowSize;

    render_group* RenderGroup = RenderCommands->RenderGroup;
    // This memory only needs to exist until the data is loaded to the GPU
    GlobalState->PhongProgram = CreatePhongProgram(RenderGroup);
    GlobalState->PhongProgramTransparent = CreatePhongTransparentProgram(RenderGroup);
    GlobalState->PlaneStarProgram = CreatePlaneStarProgram(RenderGroup);
    GlobalState->SolidColorProgram = CreateSolidColorProgram(RenderGroup);
    GlobalState->EruptionBandProgram = CreateEruptionBandProgram(RenderGroup);
    GlobalState->PhongShadingNoTexProgram = CreatePhongNoTexProgram(RenderGroup);
    GlobalState->TransparentCompositionProgram = CreateTransparentCompositionProgram(RenderGroup);
    GlobalState->GaussianProgramX = CreateGaussianBlurProgramX(RenderGroup);
    GlobalState->GaussianProgramY = CreateGaussianBlurProgramY(RenderGroup);
    GlobalState->FontRenterProgram =  CreateFontProgram(RenderGroup);
    GlobalState->ColoredSquareOverlayProgram = CreateColoredSquareOverlayProgram(RenderGroup);
    GlobalState->TexturedSquareOverlayProgram = CreateTexturedSquareOverlayProgram(RenderGroup);
    GlobalState->LineRenderProgram = CreateLineRenderProgram(RenderGroup);

    LoadMaterials();
    r32 InitTime = Platform.DEBUGGetTime();
    asset::LoadObj("..\\data\\qube.obj","Cube");
    asset::LoadObj("..\\data\\checker_plane_simple.obj", "checker_plane_simple");
    asset::LoadObj("..\\data\\sphere.obj", "Sphere");
    asset::LoadObj("..\\data\\cone.obj", "Cone");
    asset::LoadObj("..\\data\\cylinder.obj", "Cylinder");
    asset::LoadObj("..\\data\\triangle.obj", "Triangle");
    asset::LoadObj("..\\data\\plane.obj", "Plane");
//    asset::LoadObj("..\\data\\maquetiiillla.obj", "Test2");
    Platform.DEBUGPrint("Total load time %f sec\n", Platform.DEBUGGetTime() - InitTime);
    Load32BitColorTexture("Brick Wall", "..\\data\\textures\\brick_wall_base.tga");
    Load32BitColorTexture("Faded Ray", "..\\data\\textures\\faded_ray.tga");
    Load32BitColorTexture("Earth Map", "..\\data\\textures\\8081_earthmap4k.tga");
    asset::render_group* PlaneMesh = (asset::render_group*) asset::Find(asset::type::RENDER_GROUP, "checker_plane_simple");
    u32 PlaneTexHandle = PlaneMesh->Elements[0].Material->MapKdHandle;
    asset::image* PlaneTex = (asset::image*) asset::Find(asset::type::IMAGE, PlaneTexHandle);
    ecs::render::LoadImageToGpu(PlaneTexHandle, PlaneTex);

    GlobalState->ImguiContext.Icons = LoadImguiIcons(RenderGroup);
    GlobalState->ApplicationImgui = CreateApplicationImgui(GlobalPersistentArena, &GlobalState->ImguiContext, GlobalState->ColorTable.ColorCount);



    GlobalState->Initialized = true;

    {

      debug_read_file_result ReadResult = Platform.DEBUGPlatformReadEntireFile("C:\\Users\\jh\\Desktop\\box.gltf");

#if 0
      char BoxPath[] = "C:\\Users\\jh\\Desktop";
      char BoxName[] = "box.gltf";
#else
      char BoxPath[] = "C:\\Users\\jh\\Desktop\\BoxTextured\\glTF";
      char BoxName[] = "BoxTextured.gltf";
#endif
      gltf::raw_gltf_data Gltf = gltf::Load(BoxPath, BoxName,
        [](const char* Path, size_t* Size){
          debug_read_file_result ReadResult = Platform.DEBUGPlatformReadEntireFile(Path);
          *Size = ReadResult.ContentSize;
          return ReadResult.Contents;
        },
        [](void* FileDataToFree){
          Platform.DEBUGPlatformFreeFileMemory(FileDataToFree);
        });

      asset::gltf_tmp::LoadToAssetManager(&Gltf);

      gltf::Free(&Gltf);
    }





    GlobalState->Camera = {};
    InitiateCamera(&GlobalState->Camera, 70, GlobalState->World.RenderSystem->WindowSize.ApplicationAspectRatio, 0.1);
    LookAt(&GlobalState->Camera, V3(0,0,4), V3(0,0,0));
 
    GlobalState->RandomGenerator = RandomGenerator(Input->RandomSeed);
   
    { // Create some entities
      #if 1
      { // Checker Floor
        ecs::entity_id Entity = NewEntity(GlobalState->World.EntityManager, 0, "Checkered Floor", ecs::flag::RENDER | ecs::flag::COLLIDER);
        ecs::position::component* Position = GetPositionComponent(&Entity);
        ecs::position::Set(Position, V3(0,-1.1,0),  0, V3(0,1,0), V3(10,1,10));
        
        Initiate(asset::ToKey(asset::type::RENDER_GROUP, "checker_plane_simple"), GetRenderComponent(&Entity));

        ecs::collider::component* Collider = GetColliderComponent(&Entity);
        asset::mesh* Mesh = (asset::mesh*) asset::Find(asset::type::MESH, "checker_plane_simple");
        ecs::collider::Init(Collider, Mesh);
      }

      { // Transparent Cube
        ecs::entity_id Entity = NewEntity(GlobalState->World.EntityManager, 0, "Transparent Cube", ecs::flag::RENDER | ecs::flag::COLLIDER );
        ecs::position::component* Position = GetPositionComponent(&Entity);
        ecs::position::Set(Position, V3(2,0,0), 0, V3(0,1,0), V3(1,1,1));
        ecs::render::Init(asset::ToKey(asset::type::MESH, "Cube"), asset::ToKey(asset::type::PHONG_MATERIAL, "ruby"), GetRenderComponent(&Entity));
        
        ecs::collider::component* Collider = GetColliderComponent(&Entity);
        asset::mesh* Mesh = (asset::mesh*) asset::Find(asset::type::MESH, "Cube");
        ecs::collider::Init(Collider, Mesh);

      }
      
      { // Transparent Cone
        ecs::entity_id Entity = NewEntity(GlobalState->World.EntityManager, 0, "Transparent Cone", ecs::flag::RENDER | ecs::flag::COLLIDER );
        ecs::position::component* Position = GetPositionComponent(&Entity);
        ecs::position::Set(Position, V3(0,0,2), 0, V3(0,1,0), V3(1,1,1));
        ecs::render::Init(asset::ToKey(asset::type::MESH, "Cone"), asset::ToKey(asset::type::PHONG_MATERIAL, "emerald"), GetRenderComponent(&Entity));
        
        ecs::collider::component* Collider = GetColliderComponent(&Entity);
        asset::mesh* Mesh = (asset::mesh*) asset::Find(asset::type::MESH, "Cone");
        ecs::collider::Init(Collider, Mesh);
      }
      
      { // Transparent Cylinder
        ecs::entity_id Entity = NewEntity(GlobalState->World.EntityManager, 0, "Transparent Cylinder", ecs::flag::RENDER | ecs::flag::COLLIDER );
        ecs::position::component* Position = GetPositionComponent(&Entity);
        ecs::position::Set(Position, V3(2,0,2), 0, V3(0,1,0), V3(1,1,1));
        ecs::render::Init(asset::ToKey(asset::type::MESH, "Cylinder"), asset::ToKey(asset::type::PHONG_MATERIAL, "jade"), GetRenderComponent(&Entity));
       
        ecs::collider::component* Collider = GetColliderComponent(&Entity);
        asset::mesh* Mesh = (asset::mesh*) asset::Find(asset::type::MESH, "Cylinder");
        ecs::collider::Init(Collider, Mesh);

        GlobalState->DebugSquare = PushStruct(GlobalPersistentArena, ecs::entity_id);
        *GlobalState->DebugSquare = Entity;
      }

      { // Solid Cone
        ecs::entity_id Entity = NewEntity(GlobalState->World.EntityManager, 0, "Solid Cone", ecs::flag::RENDER | ecs::flag::COLLIDER );
        ecs::position::component* Position = GetPositionComponent(&Entity);
        ecs::position::Set(Position, V3(0,0,0), 0, V3(0,1,0), V3(1,1,1));
        ecs::render::Init(asset::ToKey(asset::type::MESH, "Cone"), asset::ToKey(asset::type::PHONG_MATERIAL, "silver"), GetRenderComponent(&Entity));

        ecs::collider::component* Collider = GetColliderComponent(&Entity);
        asset::mesh* Mesh = (asset::mesh*) asset::Find(asset::type::MESH, "Cone");
        ecs::collider::Init(Collider, Mesh);
      }
      #endif
#if 0
      { // TestBuilding
        asset::LoadObj("C:\\Users\\jh\\Desktop\\Donut\\Donut_grouping.obj", "Test");
        asset::render_group* RenderGroup = (asset::render_group*) asset::Find(asset::type::RENDER_GROUP, "Test");
        for (int i = 0; i <  RenderGroup->ElementCount; ++i)
        //for (int i = 0; i <  5; ++i)
        {
          asset::render_group_element* Element = RenderGroup->Elements + i;
          if(Element->Mesh)
          {

            c8 NameBuf[256] = {};
            FormatString(NameBuf, ArrayCount(NameBuf), "Test_%d", i);
            ecs::entity_id Entity = NewEntity(GlobalState->World.EntityManager, 0, NameBuf, ecs::flag::RENDER);
            ecs::position::component* Position = GetPositionComponent(&Entity);
            ecs::position::Set(Position, V3(0,0,0), 0, V3(0,1,0), V3(1,1,1));
            Initiate(Element, GetRenderComponent(&Entity));

            /*
            asset::mesh* Mesh = Element->Mesh;
            v3 MidPoint = (Mesh->AABB.P0 + Mesh->AABB.P1)*0.5;
            for (int i = 0; i < Mesh->vCount; ++i)
            {
              Mesh->v[i] -= MidPoint;
            }
            Mesh->AABB.P0 -= MidPoint;
            Mesh->AABB.P1 -= MidPoint;
            ecs::collider::Init(GetColliderComponent(&Entity), Mesh);
            */
          }
        }
      }
#endif
    }
    
  }else{
    ecs::render::Begin();
    ResetRenderGroup(RenderCommands->RenderGroup);
  }

  //Platform.DEBUGPrint("%d, %d, %d\n", Square->EntityID, Square->ChunkListIndex, GetBlockCount(&GlobalState->World.EntityManager->EntityList));

/*
  ecs::position::component* Position = GetPositionComponent(GlobalState->DebugSquare);
  //ecs::position::Set(Position, Position->RelativePosition, RotateQuaternion(0, V3(1,0,0)));
  ecs::position::Set(Position, Position->RelativePosition, QuaternionMultiplication(Position->RelativeRotation, RotateQuaternion(0.01, V3(0,1,0))), Position->Scale);
*/
  ecs::render::window_size_pixel* Window = &GlobalState->World.RenderSystem->WindowSize;
  ecs::render::SetWindowSize(GlobalState->World.RenderSystem, RenderCommands);
  CreateFrameBuffer(RenderCommands->RenderGroup, ecs::render::FrameBuffer(ecs::render::data::FRAMEBUFFER_DEFAULT),  Window->WindowWidth, Window->WindowHeight, 0, 0, 0, 0);
  
  if((ImguiNoneSelected() && ImguiIsInactive())|| ImguiIsDragging())
  {
    SceneInput(&GlobalState->Camera, Input);
  }

  aabb_tree aabbTree = BuildBroadPhaseTree();
  


  render_group* RenderGroup = RenderCommands->RenderGroup;
  if(( jwin::Pushed(Input->Keyboard.Key_ENTER) && jwin::Active(Input->Keyboard.Key_LSHIFT) && jwin::Active(Input->Keyboard.Key_LCTRL) ) || Input->ExecutableReloaded)
  {
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
    CompileShader(RenderGroup, GlobalState->PhongShadingNoTexProgram,
      1, LoadFileFromDisk("..\\jwin\\shaders\\PhongVertexCameraViewNoTex.glsl"),
      1, LoadFileFromDisk("..\\jwin\\shaders\\PhongFragmentCameraViewNoTex.glsl"));
    CompileShader(RenderGroup, GlobalState->LineRenderProgram,
      1, LoadFileFromDisk("..\\jwin\\shaders\\SolidLineProgramVertex.glsl"),
      1, LoadFileFromDisk("..\\jwin\\shaders\\SolidLineProgramFragment.glsl"));
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
  DrawAllRenderObjects();
  DrawOverlayObjects();
  ecs::render::DrawLine3D(V3(0,0,0), V3(1,1,1), V4(0,1,0,1), 0.1);
#endif
  #if 1
  ecs::render::NewRenderLevel(GetRenderSystem());
  DrawColorList(&GlobalState->ApplicationImgui);
  ecs::render::NewRenderLevel(GetRenderSystem());
  DrawEntityList(&GlobalState->ApplicationImgui);
  #endif
  ImguiEnd();
  ecs::render::Draw(GetEntityManager(), GetRenderSystem(), GlobalState->Camera.P, GlobalState->Camera.V);  
} 
