#pragma once

#include "imgui.h"
#include "ecs/entity_components.h"
#include "ecs/entity_components_backend.h"
#include "containers/chunk_list.h"

struct color_list_data {
  imgui_id* ImguiIDs; // ColorListIndeces
  s32* ColorIDs;      // Mapping IDS from Colors in the ColorTable to list indeces.
  imgui_id TextInputID;
  imgui_text_input_buffer TextInputBuffer;
  imgui_scrollable_list   ColorList;
  imgui_bordered_window   BorderWindow;
};

struct position_component_data {
  imgui_id ComponentID;

  imgui_id PosX;
  imgui_text_input_buffer TextBufferPosX;

  imgui_id PosY;
  imgui_text_input_buffer TextBufferPosY;

  imgui_id PosZ;
  imgui_text_input_buffer TextBufferPosZ;

  imgui_id RotX;
  imgui_text_input_buffer TextBufferRotX;

  imgui_id RotY;
  imgui_text_input_buffer TextBufferRotY;

  imgui_id RotZ;
  imgui_text_input_buffer TextBufferRotZ;

  // Each TextBuffer is reserved 512 bytes of data, 512 x 6 = 3072.
  utf8_byte TextData[3072];
};

struct imgui_entity_data {
  imgui_id ImguiID;
  ecs::entity_id EntityID;
  b32 Open;

  position_component_data* PositionComponentData;
};

struct menu_entity_list {
  imgui_scrollable_list EntityList;
  imgui_bordered_window BorderWindow;
  
  // Cached menu data
  chunk_list EntityData; // imgui_entity_data
  chunk_list PositionComponentData; // position_component_data
};

struct menu_entity_row {
  ecs::entity_id EntityID;
  imgui_id ImguiID;
  b32 Open;
};

typedef cmn::n_tree<menu_entity_row> me_tree;
typedef cmn::n_tree<menu_entity_row>::node me_node;
typedef cmn::n_tree<menu_entity_row>::pre_order_iterator me_iterator;

struct menu_entity_tree {
  imgui_scrollable_list EntityList;
  imgui_bordered_window BorderWindow;
  me_tree EntityTree;
};

struct application_imgui {
  menu_entity_list* MenuEntityList;
  color_list_data*  ColorListData;
  menu_entity_tree* MenuEntityTree;
};

application_imgui CreateApplicationImgui(memory_arena* Arena, imgui_context* ImguiContext, u32 ColorCount);
void DrawColorList(application_imgui* AppImgui);
void DrawEntityList(application_imgui* AppImgui);
void DrawEntityTree(application_imgui* AppImgui);