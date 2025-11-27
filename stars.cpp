#include "stars.h"

#include "platform/jwin_platform_memory.cpp"
#include "platform/jwin_platform_input.h"
#include "platform/jfont.cpp"
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
#include "imgui/imgui.cpp"
#include "imgui/application_imgui.cpp"
#include "broad_phase_collision_tree.cpp"
#include "asset_manager/asset_manager.cpp"
#include "dynamic_aabb_tree.cpp"
#include "ecs/components/component_collider.h"
#include "io/obj.cpp"
#include "render/render.cpp"
#include "render/font.cpp"
#include "containers/linked_memory_unit_tests.h"
#include "io/gltf.h"
#include "asset_manager/gltf_mapper.h"
#include "asset_manager/load_asset_files.h"

void LoadMaterial(u32 MapKdHandle, v4 Ambient, v4 Diffuse, v4 Specular, r32 Shininess, const c8* UniqueName)
{
  asset::phong_material Material = {};
  Material.Ka = &Ambient;
  Material.Kd = &Diffuse;
  Material.Ks = &Specular;
  Material.Ns = &Shininess;

  asset::image* Image = (asset::image*) asset::Find(asset::type::IMAGE, MapKdHandle);
  Assert(Image);

  Material.HasDiffuseTexture = true;
  Material.DiffuseTexture = asset::DefaultTexture(MapKdHandle);
  asset::LoadPhongMaterial(UniqueName, &Material);
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
  asset::key TextureKey = 0;
  asset::LoadImage("WhitePixel", "N/A", "N/A", &WhitePixelBitmap, &TextureKey);


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


world InitiateWorld(application_render_commands* RenderCommands)
{
  world Result = {};
  Result.EntityManager = ecs::CreateEntityManager();
  Result.PositionSystem = ecs::position::CreatePositionSystem();
  Result.Renderer = render::CreateRenderer(RenderCommands->RenderGroup, RenderCommands->WindowInfo.Width, RenderCommands->WindowInfo.Height, RenderCommands);

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
    ecs::entity_id EntityID = ecs::GetEntityID(&EntityIterator);
    ecs::render::component* Component = GetRenderComponent(&EntityIterator);
    if(Component->RenderTreeHandle)
    {
      render::DrawRenderTree(Component->RenderTreeHandle);
    }else if(Component->MeshID){

      asset::mesh* Mesh = (asset::mesh*) Find(asset::type::MESH, Component->MeshID);
      asset::phong_material* PhongMaterial = (asset::phong_material*) Find(asset::type::PHONG_MATERIAL, Component->PhongMaterialHandle);

      ecs::position::component* Position = GetPositionComponent(&EntityIterator);
      m4 Transform = M4Identity();
      if(Position)
      {
        Transform = GetModelMatrix(Position);
      }
      for (int i = 0; i < Mesh->PrimitiveCount; ++i)
      {
        render::DrawAssetRenderObject(Mesh->Primitives + i, PhongMaterial, Transform);
      }
    }
    
  }
}

asset::mesh* MeshFromTree(const c8* Name)
{
  asset::package* Package = (asset::package*) asset::Find(asset::type::PACKAGE, Name);
  asset::render_tree* Tree = (asset::render_tree*) asset::Find(asset::type::RENDER_TREE, Name);
  Assert(Tree->NodeCount() == 1);
  asset::mesh* Mesh = (asset::mesh*) asset::Find(asset::type::MESH, Tree->m_root->Data->Mesh);
  return Mesh;
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

// Mesh ID 3122231961 2 Primitives
void LoadAndRenderGLTFEngine()
{
  if(!GlobalState->DebugPackage)
  {
    asset::package_id PackageID = asset::Load("..\\data\\gltf\\2CylinderEngine\\2CylinderEngine.gltf", "2CylinderEngine");
    //asset::package_id PackageID = asset::Load("..\\data\\gltf\\testbox\\box.gltf", "testbox");
    //asset::package_id PackageID = asset::Load("..\\data\\gltf\\BoxTextured\\BoxTextured.gltf", "BoxTextured");
    //asset::package_id PackageID = asset::Load("..\\data\\gltf\\testsphere\\testsphere.gltf", "testsphere");
    GlobalState->DebugPackage = (asset::package*) asset::Find(asset::type::PACKAGE, PackageID);
  }

  for (int i = 0; i < GlobalState->DebugPackage->RenderTreeCount; ++i)
  {
    render::DrawRenderTree(GlobalState->DebugPackage->RenderTrees[i]);
    //ecs::render::DrawRenderTree(GlobalState->DebugPackage->RenderTrees[i]);
  }
}

file_local inline void Initiate(asset::mesh_id MeshKey, asset::phong_material_id MaterialKey, ecs::render::component* Render)
{
  asset::mesh* Mesh = (asset::mesh*) asset::Find(asset::type::MESH, MeshKey);
  Render->MeshID = MeshKey;
  Render->PhongMaterialHandle = MaterialKey;
}


file_local inline void Initiate(asset::render_tree_id RenderTreeKey, ecs::render::component* Render)
{
  asset::render_tree* Tree = (asset::render_tree*) asset::Find(asset::type::RENDER_TREE, RenderTreeKey);
  Assert(Tree->NodeCount() == 1); // We don't support rendering big node hierarchy (yet)
  Assert(Tree->m_root->Data->Mesh);

  asset::mesh* Mesh = (asset::mesh*) asset::Find(asset::type::MESH, Tree->m_root->Data->Mesh);

  if(Mesh->PrimitiveCount && Mesh->Primitives->PhongMaterial)
  {
    Render->MeshID = Tree->m_root->Data->Mesh;
    Render->PhongMaterialHandle = Mesh->Primitives->PhongMaterial;
  }else if(Mesh->Primitives->PbrMaterial){
    Render->RenderTreeHandle = RenderTreeKey;
  }
}


// void ApplicationUpdateAndRender(application_memory* Memory, application_render_commands* RenderCommands, jwin::device_input* Input)
extern "C" JWIN_UPDATE_AND_RENDER(ApplicationUpdateAndRender)
{
  GlobalState          = JwinBeginFrameMemory(application_state);
  GlobalInput          = Input;
  GlobalImguiContext   = &GlobalState->ImguiContext;
  GlobalRenderCommands = RenderCommands;
  GlobalRenderer       = GlobalState->World.Renderer;
  GlobalAssetManager   = GlobalState->AssetManager;
  GlobalEntityManager  = GlobalState->World.EntityManager;
  GlobalPositionSystem = &GlobalState->World.PositionSystem;
  GlobalWindowSize     = WindowSizePixel(RenderCommands, RenderCommands->WindowInfo.Width, RenderCommands->WindowInfo.Height);

  ResetRenderGroup(RenderCommands->RenderGroup);
  platform_offscreen_buffer* OffscreenBuffer = &RenderCommands->PlatformOffscreenBuffer;
  ImguiBegin(Input);
  GlobalTime = Input->Time;

  asset::key BoxTextured = {};

  if(!GlobalState->Initialized)
  {
    //PowerOfTwoMiddles(1000000000);
    GlobalState->ColorTable = menu::CreateColorTable(GlobalPersistentArena);
    GlobalState->AssetManager = asset::CreateAssetManager();
    GlobalAssetManager = GlobalState->AssetManager;

    RenderCommands->RenderGroup = InitiateRenderGroup();
    GlobalState->World   = InitiateWorld(RenderCommands);
    GlobalPositionSystem = &GlobalState->World.PositionSystem;
    GlobalEntityManager  = GlobalState->World.EntityManager;
    GlobalRenderer       = GlobalState->World.Renderer;

    LinkedMemoryUnitTests(GlobalTransientArena);

    window_size_pixel* Window = &GlobalWindowSize;

    render_group* RenderGroup = RenderCommands->RenderGroup;

    LoadMaterials();
    r32 InitTime = Platform.DEBUGGetTime();
    asset::Load("..\\data\\qube.obj","Cube");
    asset::Load("..\\data\\checker_plane_simple.obj", "checker_plane_simple");
    asset::Load("..\\data\\sphere.obj", "Sphere");
    asset::Load("..\\data\\cone.obj", "Cone");
    asset::Load("..\\data\\cylinder.obj", "Cylinder");
    asset::Load("..\\data\\triangle.obj", "Triangle");
    asset::Load("..\\data\\plane.obj", "Plane");
    Platform.DEBUGPrint("Total load time %f sec\n", Platform.DEBUGGetTime() - InitTime);

    GlobalState->ImguiContext.Icons = LoadImguiIcons(RenderGroup);
    GlobalState->ApplicationImgui = CreateApplicationImgui(GlobalPersistentArena, &GlobalState->ImguiContext, GlobalState->ColorTable.ColorCount);

    GlobalState->Initialized = true;

    GlobalState->Camera = {};
    InitiateCamera(&GlobalState->Camera, 70, GlobalWindowSize.ApplicationAspectRatio, 0.1);
    LookAt(&GlobalState->Camera, V3(0,0,4), V3(0,0,0));
 
    GlobalState->RandomGenerator = RandomGenerator(Input->RandomSeed);
    

    { // Create some entities

      { // Checker Floor
        asset::mesh* Mesh = MeshFromTree("checker_plane_simple");

        ecs::entity_id Entity = NewEntity(GlobalState->World.EntityManager, 0, "Checkered Floor", ecs::flag::RENDER | ecs::flag::COLLIDER);
        GlobalState->FloorEntity = Entity;
        ecs::position::component* Position = GetPositionComponent(&Entity);
        ecs::position::InitiatePosition(V3(0,-1.1,0),  0, V3(0,1,0), V3(10,1,10), &Entity);

        Initiate(asset::ToKey(asset::type::RENDER_TREE, "checker_plane_simple"), GetRenderComponent(&Entity));
        ecs::collider::component* Collider = GetColliderComponent(&Entity);
        ecs::collider::Init(Collider, Mesh);
      }

      { // Transparent Cube
        asset::mesh* Mesh = MeshFromTree("Cube");

        ecs::entity_id Entity = NewEntity(GlobalState->World.EntityManager, 0, "Transparent Cube", ecs::flag::RENDER | ecs::flag::COLLIDER );
        ecs::position::InitiatePosition(V3(2,0,0), 0, V3(0,1,0), V3(1,1,1), &Entity, &GlobalState->FloorEntity);

        Initiate(asset::ToHeader(Mesh)->Key, asset::ToKey(asset::type::PHONG_MATERIAL, "ruby"), GetRenderComponent(&Entity));
        ecs::collider::component* Collider = GetColliderComponent(&Entity);
        ecs::collider::Init(Collider, Mesh);
      }


      { // Transparent Cone
        asset::mesh* Mesh = MeshFromTree("Cone");

        ecs::entity_id Entity = NewEntity(GlobalState->World.EntityManager, 0, "Transparent Cone", ecs::flag::RENDER | ecs::flag::COLLIDER );
        ecs::position::InitiatePosition(V3(0,0,2), 0, V3(0,1,0), V3(1,1,1), &Entity, &GlobalState->FloorEntity);
        Initiate(asset::ToHeader(Mesh)->Key, asset::ToKey(asset::type::PHONG_MATERIAL, "emerald"), GetRenderComponent(&Entity));
        
        
        ecs::collider::component* Collider = GetColliderComponent(&Entity);
        ecs::collider::Init(Collider, Mesh);
      }
      
      { // Transparent Cylinder
        asset::mesh* Mesh = MeshFromTree("Cylinder");

        ecs::entity_id Entity = NewEntity(GlobalState->World.EntityManager, 0, "Transparent Cylinder", ecs::flag::RENDER | ecs::flag::COLLIDER );
        ecs::position::InitiatePosition(V3(2,0,2), 0, V3(0,1,0), V3(1,1,1), &Entity, &GlobalState->FloorEntity);

        Initiate(asset::ToHeader(Mesh)->Key, asset::ToKey(asset::type::PHONG_MATERIAL, "jade"), GetRenderComponent(&Entity));
       
        
        ecs::collider::component* Collider = GetColliderComponent(&Entity);
        ecs::collider::Init(Collider, Mesh);
      }

      { // Solid Cone
        asset::mesh* Mesh = MeshFromTree("Cone");

        ecs::entity_id Entity = NewEntity(GlobalState->World.EntityManager, 0, "Solid Cone", ecs::flag::RENDER | ecs::flag::COLLIDER );
        ecs::position::InitiatePosition(V3(0,0,0), 0, V3(0,1,0), V3(1,1,1), &Entity, &GlobalState->FloorEntity);

        Initiate(asset::ToHeader(Mesh)->Key, asset::ToKey(asset::type::PHONG_MATERIAL, "silver"), GetRenderComponent(&Entity));

        ecs::collider::component* Collider = GetColliderComponent(&Entity);
        ecs::collider::Init(Collider, Mesh);
      }
    }    
  }else{
    render::Begin();
    ResetRenderGroup(RenderCommands->RenderGroup);
  }

  ecs::position::component* FloorPos = GetPositionComponent(&GlobalState->FloorEntity);
  FloorPos->RelativeRotation = QuaternionMultiplication(FloorPos->RelativeRotation, RotateQuaternion( 0.01, V3(0,0,1)));
//  Platform.DEBUGPrint("%f\n", Norm(FloorPos->RelativeRotation));

  LoadAndRenderGLTFEngine();
  
  if((ImguiNoneSelected() && ImguiIsInactive())|| ImguiIsDragging())
  {
    SceneInput(&GlobalState->Camera, Input);
  }

  if(( jwin::Pushed(Input->Keyboard.Key_ENTER) && jwin::Active(Input->Keyboard.Key_LSHIFT) && jwin::Active(Input->Keyboard.Key_LCTRL) ) || Input->ExecutableReloaded)
  {
    render::RecompileAllPrograms();
  }

  ecs::position::UpdatePositions();

  UpdateViewMatrix(&GlobalState->Camera);
  DrawAllRenderObjects();

  render::NewOverlayLevel();
  DrawColorList(&GlobalState->ApplicationImgui); // LEaky
  render::NewOverlayLevel();
  DrawEntityList(&GlobalState->ApplicationImgui);
  ImguiEnd();
  //if(GlobalRenderer->ActiveCamera)
  //{
  //  render::RenderScene(GlobalRenderer->ActiveCamera->P, GlobalRenderer->ActiveCamera->V);
  //}else{
    render::RenderScene(GlobalState->Camera.P, GlobalState->Camera.V);
  //}
}
