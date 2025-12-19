#pragma once

#include "ecs/entity_components.h"
#include "ecs/entity_components_backend.h"
#include "containers/chunk_list.h"
#include "imgui_border_window.h"
#include "imgui_scrollbar.h"

namespace imgui { 

struct menu_entity_component_id {
  ecs::flag::component_type Type;
  id ImguiID;
};

inline menu_entity_component_id ImguiEntityComponent(ecs::flag::component_type Type){
  menu_entity_component_id Result = {};
  Result.Type  = Type;
  Result.ImguiID  = NewButtonID();
  return Result;
}

struct menu_entity_row {
  ecs::entity_id EntityID;
  cmn::vector<menu_entity_component_id> ComponentImguiIDs;
  id ImguiID;
  b32 Open;
};

typedef cmn::n_tree<menu_entity_row> me_tree;
typedef cmn::n_tree<menu_entity_row>::node me_node;
typedef cmn::n_tree<menu_entity_row>::pre_order_iterator me_iterator;

struct menu_entity_tree {
  imgui_vertical_scrollbar VerticalScrollbar;
  imgui_bordered_window BorderWindow;
  me_tree EntityTree;
};

void PushNewEntity(me_tree* MenuEntityTree, me_node* MenuParent, ecs::entity_id* NewEntity);
void DrawEntityTree(application_menu* AppImgui);

} // namespace imgui