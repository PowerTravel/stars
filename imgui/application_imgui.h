#pragma once

#include "imgui.h"
#include "imgui_scrollbar.h"

namespace imgui { 

struct application_imgui {
  struct menu_color_list*  ColorListData;
  struct menu_entity_tree* MenuEntityTree;
};

application_imgui CreateApplicationImgui(memory_arena* Arena, imgui_context* ImguiContext, u32 ColorCount);


} // namespace imgui