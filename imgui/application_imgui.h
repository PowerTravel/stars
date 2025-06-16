#pragma once

#include "imgui.h"
#include "ecs/entity_components.h"
#include "ecs/entity_components_backend.h"
#include "ecs/systems/system_render.h"
#include "containers/chunk_list.h"

struct color_list_data {
  imgui_id* ImguiIDs; // ColorListIndeces
  s32* ColorIDs;      // Mapping IDS from Colors in the ColorTable to list indeces.

  imgui_text_input_buffer TextInputBuffer;
  imgui_scrollable_list ColorList;
  imgui_bordered_window BorderWindow;
};


struct imgui_entity_data {
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
  chunk_list EntityData; // imgui_entity_data
};

struct application_imgui {
  menu_entity_list* MenuEntityList;
  color_list_data*  ColorListData;
};

application_imgui CreateApplicationImgui(memory_arena* Arena, imgui_context* ImguiContext, u32 ColorCount);
void DrawColorList(application_imgui* AppImgui);
void DrawEntityList(application_imgui* AppImgui);