#pragma once

#include "externals\json.hpp"

#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STB_IMAGE_IMPLEMENTATION
//#define STBI_NO_FAILURE_STRINGS
#include "externals/stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION

#include "platform/jwin_platform.h"
#include "platform/jfont.h"
#include "commons/random.h"
#include "camera.h"
#include "io/obj.h"
#include "containers/chunk_list.h"
#include "ecs/entity_components.h"
#include "ecs/systems/system_render.h"
#include "menu/color_table.h"
#include "imgui/imgui.h"
#include "imgui/application_imgui.h"
#include "asset_manager/asset_manager.h"
typedef void(*func_ptr_void)(void); 

#define DEBUGPrintRect(Rect) Platform.DEBUGPrint("%1.2f,%1.2f,%1.2f,%1.2f\n",(Rect).X, (Rect).Y ,(Rect).W, (Rect).H);

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
};

struct application_state
{
  b32 Initialized;
  camera Camera;

  random_generator RandomGenerator;
  asset::manager* AssetManager;

  u32 PhongProgram;
  u32 PhongShadingNoTexProgram;
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
  u32 TexturedSquareOverlayProgram;
  u32 LineRenderProgram;
  u32 BRDFProgram;

  // Key is Asset Index
  // Value is u32, Handle from the render system

  u32 Skybox;

  function_pool* FunctionPool;
  menu::color_table ColorTable;
  world World;

  imgui_context ImguiContext;
  application_imgui ApplicationImgui;

  ecs::entity_id* DebugSquare;
};

global_variable ecs::entity_manager* GlobalEntityManager = 0;
global_variable application_render_commands* GlobalRenderCommands = 0;
global_variable application_state* GlobalState = 0;
global_variable jwin::device_input* GlobalInput = 0;
global_variable imgui_context* GlobalImguiContext = 0;
global_variable asset::manager* GlobalAssetManager = 0;
global_variable ecs::render::system* GlobalRenderSystem = 0;
global_variable float GlobalTime = 0;

// Global Singleton Getters
inline ecs::render::system* GetRenderSystem() {
  return GlobalState->World.RenderSystem;
}

inline ecs::entity_manager* GetEntityManager() {
  return GlobalState->World.EntityManager;
}

inline menu::color_table* GetColorTable()
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

