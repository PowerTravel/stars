#pragma once

#include "menu/imgui/imgui.h"
#include "menu/imgui/scrollbar.h"

namespace imgui { 
namespace app {
struct menu {
  struct color_list*  ColorListData;
  struct entity_tree* MenuEntityTree;
};

menu CreateAppllicationMenu(memory_arena* Arena, context* ImguiContext, u32 ColorCount);

} // namespace app
} // namespace imgui