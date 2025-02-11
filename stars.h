#pragma once
#include "platform/jwin_platform.h"
#include "platform/jfont.h"
#include "commons/random.h"
#include "camera.h"
#include "debug_draw.h"
#include "containers/chunk_list.h"
#include "ecs/entity_components.h"
#include "ecs/systems/system_render.h"
#include "menu/menu_interface.h"
#include "menu/color_table.h"
typedef void(*func_ptr_void)(void);

struct function_ptr
{
  c8* Name;
  func_ptr_void Function;
};

struct function_pool
{
  u32 Count;
  function_ptr Functions[256];
};


struct world {
  ecs::entity_manager* EntityManager;
  ecs::render::system* RenderSystem;
  menu_interface* MenuInterface;
  container_node* ScenePlugin;
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
  u32 ColoredSquareOverlayProgram;

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

  debug_application_render_commands* DebugRenderCommands;

  function_pool* FunctionPool;
  menu::color_table ColorTable;
  world World;
};

debug_application_render_commands* GlobalDebugRenderCommands = 0;
application_state* GlobalState = 0;

// Global Singleton Getters
ecs::render::system* GetRenderSystem() {
  return GlobalState->World.RenderSystem;
}

ecs::entity_manager* GetEntityManager() {
  return GlobalState->World.EntityManager;
}

menu::color_table* GetColorTable()
{
  return &GlobalState->ColorTable;
}

inline func_ptr_void* _DeclareFunction(func_ptr_void Function, const c8* Name)
{
  Assert(GlobalState);
  function_pool* Pool = GlobalState->FunctionPool;
  Assert(Pool->Count < ArrayCount(Pool->Functions))
    function_ptr* Result = Pool->Functions;
  u32 FunctionIndex = 0;
  while(Result->Name && !jstr::ExactlyEquals(Result->Name, Name))
  {
    Result++;
  }
  if(!Result->Function)
  {
    Assert(Pool->Count == (Result - Pool->Functions))
      Pool->Count++;
    Result->Name = (c8*) PushCopy(GlobalPersistentArena, (jstr::StringLength(Name)+1)*sizeof(c8), (void*) Name);
    Result->Function = Function;
  }else{
    Result->Function = Function;
  }
  return &Result->Function;
}


#define DeclareFunction(Type, Name) (Type**) _DeclareFunction((func_ptr_void) (&Name), #Name )
#define CallFunctionPointer(PtrToFunPtr, ... ) (**PtrToFunPtr)(__VA_ARGS__)
#include "menu/function_pointer_pool.h"
