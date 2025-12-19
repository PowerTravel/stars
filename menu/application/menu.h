#pragma once

#include "menu/imgui/imgui.h"
#include "menu/imgui/scrollbar.h"

namespace imgui { 

struct application_menu {
  struct menu_color_list*  ColorListData;
  struct menu_entity_tree* MenuEntityTree;
};

application_menu CreateApplicationImgui(memory_arena* Arena, context* ImguiContext, u32 ColorCount);


} // namespace imgui