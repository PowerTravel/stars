#pragma once

#include "ecs/entity_components.h"
#include "ecs/entity_components_backend.h"
#include "containers/chunk_list.h"
#include "menu/imgui/border_window.h"
#include "menu/imgui/scrollbar.h"

namespace imgui { 
namespace app {

struct entity_component_id {
  ecs::flag::component_type Type;
  id ImguiID;
};

inline entity_component_id ImguiEntityComponent(ecs::flag::component_type Type){
  entity_component_id Result = {};
  Result.Type  = Type;
  Result.ImguiID  = NewButtonID();
  return Result;
}

struct entity_row {
  ecs::entity_id EntityID;
  cmn::vector_lm<entity_component_id> ComponentImguiIDs;
  id ImguiID;
  b32 Open;
};

typedef cmn::n_tree_lm<entity_row> me_tree;
typedef cmn::n_tree_lm<entity_row>::node me_node;
typedef cmn::n_tree_lm<entity_row>::pre_order_iterator<TransientAllocators> me_iterator;

struct entity_list {
  imgui_vertical_scrollbar VerticalScrollbar;
  imgui_bordered_window BorderWindow;
  me_tree EntityTree;
};

entity_list* CreateEntityList(memory_arena* Arena);
void PushNewEntity(me_tree* MenuEntityTree, me_node* MenuParent, ecs::entity_id* NewEntity);
void DrawEntityTree(menu* AppImgui);
void LoadNewEntitiesToEntityList();
} // namespace app
} // namespace imgui