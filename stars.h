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
#include "platform/jwin_debug.h"
#include "commons/random.h"
#include "camera.h"
#include "io/obj.h"
#include "containers/chunk_list.h"
#include "ecs/entity_components.h"
#include "ecs/systems/system_position.h"
#include "menu/imgui/color_table.h"
#include "menu/imgui/imgui.h"
#include "menu/imgui/border_window.h"
#include "menu/app/menu.h"
#include "asset_manager/asset_manager.h"
#include "render/render.h"
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
  render::renderer* Renderer;
};

struct application_state
{
  b32 Initialized;
  camera Camera;

  random_generator RandomGenerator;
  asset::manager* AssetManager;

  // Key is Asset Index
  // Value is u32, Handle from the render system

  u32 Skybox;
  function_pool* FunctionPool;
  imgui::color_table ColorTable;
  world World;

  window_size_pixel WindowSize;

  imgui::context ImguiContext;
  imgui::app::menu ApplicationMenu;

  ecs::entity_id FloorEntity;
};

global_variable ecs::entity_manager* GlobalEntityManager = 0;
global_variable application_render_commands* GlobalRenderCommands = 0;
global_variable application_state* GlobalState = 0;
global_variable jwin::device_input* GlobalInput = 0;
global_variable imgui::context* GlobalImguiContext = 0;
global_variable asset::manager* GlobalAssetManager = 0;
global_variable render::renderer* GlobalRenderer = 0;
global_variable float GlobalTime = 0;
global_variable window_size_pixel GlobalWindowSize = {};
#if JWIN_PROFILE
global_variable debug_table* GlobalDebugTable;
#endif
#if 0
#if JWIN_PROFILE
global_variable debug_table GlobalDebugTable_;
debug_table* GlobalDebugTable = &GlobalDebugTable_;
#else
debug_table* GlobalDebugTable = 0;
#endif
#endif

// Global Singleton Getter
inline render::renderer* GetRenderer() {
  return GlobalState->World.Renderer;
}

inline ecs::entity_manager* GetEntityManager() {
  return GlobalState->World.EntityManager;
}

inline imgui::color_table* GetColorTable()
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

